import 'dart:async';
import 'dart:convert';
import 'dart:math';

import 'package:flutter/services.dart';
import 'package:flutter_blue/flutter_blue.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:waveglide/ble/serial_service_config.dart';
import 'package:waveglide/data/ble_status.dart';
import 'package:waveglide/data/data_item.dart';
import 'package:waveglide/data/waveglide_device.dart';
import 'package:waveglide/util/toast_util.dart';

class BLEService {
  FlutterBlue _flutterBlue = FlutterBlue.instance;

  BluetoothDevice _waveglideDevice;
  StreamSubscription _waveglideDeviceConnection;
  StreamSubscription _waveglideDeviceStateSubscription;
  List<BluetoothService> _waveglideDeviceServices = new List();
  BluetoothDeviceState _waveglideDeviceState = BluetoothDeviceState.disconnected;
  BluetoothCharacteristic _waveglideTxCharacteristic;
  BluetoothCharacteristic _waveglideRxCharacteristic;
  StreamSubscription _waveglideRxSubscription;
  BluetoothService _waveglideSerialService;

  StreamSubscription _stateSubscription;
  StreamSubscription _scanSubscription;
  BluetoothState _bluetoothState = BluetoothState.unknown;
  bool _isScanning = false;
  String _waveglideDeviceId = "DB:10:24:FD:CF:FE";

  StreamController<DataItem> controller;
  Stream data;
  String receivedBuffer = "";
  StreamController<BleStatus> _bleStatusController = new StreamController.broadcast();

  List<BluetoothDevice> bluetoothDevices = [];
  SharedPreferences _preferences;
  WaveglideDevice _selectedWaveglideDevice;

  static final BLEService _instance = BLEService._internal();

  factory BLEService() {
    return _instance;
  }

  void _getData() async {
    _preferences = await SharedPreferences.getInstance();
    _updateSelectedDevice();
  }

  void _updateSelectedDevice() {
    if(_preferences.containsKey("waveglideDevice")) {
      _selectedWaveglideDevice = WaveglideDevice.fromJsonString(_preferences.getString("waveglideDevice")) ;
    } else {
      _selectedWaveglideDevice = null;
    }
  }

  BLEService._internal() {
    _getData();
    ToastUtil.showSimpleToast("BLEService: Init");
    _flutterBlue.state.listen((s) {
      _handleBluetoothState(s);
    });
    controller  = StreamController<DataItem>();
    data = controller.stream;
    _bleStatusController.add(BleStatus.bluetoothStateOff);
  }

  Stream<dynamic> status() {
    return _bleStatusController.stream;
  }


  startScan() async{

    if(_bluetoothState != BluetoothState.on) {
      _stopScan();
      return;
    }

    if(_isScanning) {
      return;
    }

    print("start scanning");
    _isScanning = true;
    print(_isScanning);
    _updateSelectedDevice();
    //ToastUtil.showSimpleToast("Scanning for Waveglide devices.");

    bluetoothDevices.clear();
    _bleStatusController.add(BleStatus.deviceScanning);

    _scanSubscription = _flutterBlue.scan(timeout: Duration(seconds: 10)).listen((scanResult) {

      if(!bluetoothDevices.contains(scanResult.device) && scanResult.device.name.trim().length > 0) {
        bluetoothDevices.add(scanResult.device);
        _bleStatusController.add(BleStatus.deviceScanning);
      }


      //print(scanResult.device.name + " " + scanResult.device.type.toString());
      //ToastUtil.showSimpleToast(scanResult.device.name + " " + scanResult.device.type.toString());
      /*if(scanResult.device.id.toString() == _waveglideDeviceId) {
        _waveglideDevice = scanResult.device;
        //_connect();
        _stopScan();
      }
*/
      if(_selectedWaveglideDevice != null && _waveglideDevice == null && scanResult.device.id.toString() == _selectedWaveglideDevice.deviceId) {
        _waveglideDevice = scanResult.device;
        ToastUtil.showSimpleToast("Connecting to " + scanResult.device.name);
        print("found");
        connect();
        _bleStatusController.add(BleStatus.deviceConnected);
        HapticFeedback.vibrate();
        _stopScan();
      }

    }, onDone: _stopScan());


  }

  String getConnectedDeviceName() {
    if(_waveglideDevice != null) {
      return _waveglideDevice.name;
    }
    return "-";
  }

  _stopScan() {
    /*if(_waveglideDevice == null && _scanSubscription != null) {
      ToastUtil.showSimpleToast("Waveglide device not found. Make sure location is enabled.");
    }*/
    _flutterBlue.stopScan();
    _bleStatusController.add(BleStatus.bluetoothStateOn);
    _scanSubscription?.cancel();
    _scanSubscription = null;
    _isScanning = false;

    print("stop scan");


  }


  connect() async {
    // Connect to device

    await disconnect(reset: false);

    await _waveglideDevice.connect(timeout: const Duration(seconds: 10));

    // Update the connection state immediately
    _waveglideDevice.state.listen((s) {
      _handleWaveglideDeviceState(s);
    });
  }

  _handleWaveglideDeviceState(BluetoothDeviceState s) {
    _waveglideDeviceState = s;
    if(_waveglideDeviceState == BluetoothDeviceState.connected) {
      print("connected");
      ToastUtil.showSimpleToast("Connected.");
      _bleStatusController.add(BleStatus.deviceConnected);
      //_disconnect();
      _discoverServices();
    } else if(_waveglideDeviceState == BluetoothDeviceState.disconnected) {
      print("disconnected");
      ToastUtil.showSimpleToast("Disconnected.");
      disconnect(reset: true);
      _bleStatusController.add(BleStatus.deviceDisconnected);
    }
  }


  bool isConnected() {
    return _waveglideDevice != null;
  }
  _discoverServices() {
    _waveglideDevice.discoverServices().then((s) {
      _waveglideDeviceServices = s;
      _attachSerialCharacteristics();
    });
  }

  Future<void> _attachSerialCharacteristics() async {

    if(_waveglideSerialService == null) {
      _waveglideSerialService = _waveglideDeviceServices.firstWhere((s)
      => s.uuid == NordicSerialServiceConfig().serviceId);

      _waveglideRxCharacteristic = _waveglideSerialService.characteristics.firstWhere((c)
      => c.uuid == NordicSerialServiceConfig().rxCharacteristicId);

      _waveglideTxCharacteristic = _waveglideSerialService.characteristics.firstWhere((c)
      => c.uuid == NordicSerialServiceConfig().txCharacteristicId);

      _setReceiverSubscription();
    }

  }

  _setReceiverSubscription() async {

    await _waveglideRxCharacteristic.setNotifyValue(true);
    // ignore: cancel_subscriptions
    final _serialReceiverSubscription = _waveglideRxCharacteristic.value.listen((d) {
      String received = utf8.decode(d);
      //print(received);
      //print(receivedBuffer);
      //print(1);
      if(received == null || received.length == 0) {
        return;
      } else if(received.indexOf("\n") >= 0) {
        int endPos = received.indexOf("\n");

        receivedBuffer += received.substring(0, endPos);
        print("set: " + receivedBuffer);

        _parseValues(receivedBuffer);
        if(received.length > endPos + 1) {
          receivedBuffer = received.substring(endPos + 1);
        } else {
          receivedBuffer = "";
        }

      } else {
        receivedBuffer += received;
      }
    });
    _waveglideRxSubscription = _serialReceiverSubscription;

  }

  _parseValues(String data) {
    try {
      if(data == null || data.length == 0) {
        print("invalid values");
        return;
      }
      List list = data.split(" ");
      //print(data);
      //print(list.length);
      if(list.length != 11) {
        print("invalid values count");
        return;
      }

      /*
        0 - breath_strength - average breath amplitude in % [0, 100]
        1 - period_ms - length of one respiration cycle in ms [0, 2147483647]
        2 - oxygen_pct - amount of supplemental O2 in % [0, 100]
        3 - oxygen_total - total time of oxygenation in s [0, 2147483647]
        4 - flight_level - altitude in ft divided by 100 on QNH 1013 [0, 2147483647]
        5 - bat_pct - battery level in % [0, 100]
        6 - pulsoxy_signalstrength - SPO2 probe signal [0,7]
        7 - pulsoxy_heartrate - heart beats per minute [0, 255]
        8 - pulsoxy_spo2 - blood oxygen level in % [0, 100]
        9 - pulsoxy_status - 1 byte (_, signalOK, probeOK, sensorOK, pulseOK, _, _, pulsoxyOK)
        10 - status - 1 byte (_, _, bleFAIL, baroFAIL, breathFAST, breathLOW, batLOW, spo2LOW)
       */

      controller.add(DataItem.asInt(int.parse(list[0]), DataItemType.breathStrengthPercentage));
      controller.add(DataItem.asInt(int.parse(list[1]), DataItemType.respirationPeriodMs));
      controller.add(DataItem.asInt(int.parse(list[2]), DataItemType.oxygenPercentage));
      controller.add(DataItem.asInt(int.parse(list[3]), DataItemType.oxygenTotal));
      controller.add(DataItem.asInt(int.parse(list[4]), DataItemType.flightLevel));
      controller.add(DataItem.asInt(int.parse(list[5]), DataItemType.batteryPercentage));
      controller.add(DataItem.asInt(int.parse(list[6]), DataItemType.pulsoxySignalStrength));
      controller.add(DataItem.asInt(int.parse(list[7]), DataItemType.pulsoxyHeartrate));
      controller.add(DataItem.asInt(int.parse(list[8]), DataItemType.pulsoxySpO2));
      controller.add(DataItem.asInt(int.parse(list[9]), DataItemType.pulsoxyStatus));
      controller.add(DataItem.asInt(int.parse(list[10]), DataItemType.status));

      //print(list);
    } catch(e) {
      print(e.toString());
    }
  }

 /* sendValues(List<int> values) {
    String joinedValues = values.join(',');
    sendRawData(utf8.encode('[' + joinedValues + ']'));
  }

  Future<void> sendRawData(List<int> raw) async {

    try {
      int offset = 0;

      final int chunkSize = NordicSerialServiceConfig().lengthOfCharacteristic;
      while (offset < raw.length) {
        var chunk = raw.skip(offset).take(chunkSize).toList();
        offset += chunkSize;
        await _waveglideDevice.writeCharacteristic(_waveglideTxCharacteristic, chunk,
          type: CharacteristicWriteType.withResponse);
      }
    } catch(e) {
      print("Write failed:" + e.toString());
    }
  }
*/

  Future<void> disconnect({reset = true}) async {
    // Remove all value changed listeners
    //valueChangedSubscriptions.forEach((uuid, sub) => sub.cancel());
    //valueChangedSubscriptions.clear();

    _waveglideSerialService = null;
    _waveglideTxCharacteristic = null;
    if (_waveglideRxCharacteristic != null) {
      try {
        if(_waveglideRxSubscription != null) {
          _waveglideRxSubscription.cancel();
        }
        //await _waveglideDevicesetNotifyValue(_waveglideRxCharacteristic, false);
      } catch(e) {
        print(e.toString());
      }
    }
    _waveglideRxCharacteristic = null;
    _waveglideRxSubscription?.cancel();
    _waveglideRxSubscription = null;

    _waveglideDeviceStateSubscription?.cancel();
    _waveglideDeviceStateSubscription = null;

    _waveglideDeviceConnection?.cancel();
    _waveglideDeviceConnection = null;

    _waveglideDevice?.disconnect();
    if (reset) {
      _bleStatusController.add(BleStatus.deviceDisconnected);
      _waveglideDevice = null;
    }

    print("disconnected " + reset.toString());
  }



  _handleBluetoothState(BluetoothState s) {
    _bluetoothState = s;

    if(_bluetoothState == BluetoothState.on) {
      ToastUtil.showSimpleToast("BLEService: Bluetooth enabled");
      startScan();
      _bleStatusController.add(BleStatus.bluetoothStateOn);
    } else {
      ToastUtil.showSimpleToast("BLEService: Bluetooth disabled");
      //_disconnect();
      _bleStatusController.add(BleStatus.bluetoothStateOff);
    }
  }

}

