import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:waveglide/data/waveglide_device.dart';
import 'package:waveglide/services/ble_service.dart';

class BluetoothDevicesDialog extends StatefulWidget {

  const BluetoothDevicesDialog({Key key}) : super(key: key);

  @override
  _BluetoothDevicesDialogState createState() => _BluetoothDevicesDialogState();
}

class _BluetoothDevicesDialogState extends State<BluetoothDevicesDialog> {
  SharedPreferences _preferences;
  WaveglideDevice _waveglideDevice;
  BLEService _bleService = BLEService();
  @override
  void initState() {
    super.initState();
    _getData();
    _bleService.startScan();
    _bleService.status().listen((status) {
      if(!mounted) {
        return;
      }
      setState(() {

      });
    });
  }

  void _getData() async {
    _preferences = await SharedPreferences.getInstance();

    setState((){
      if(_preferences.containsKey("waveglideDevice")) {
        _waveglideDevice = WaveglideDevice.fromJsonString(_preferences.getString("waveglideDevice")) ;
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    List<Widget> devices = [];
    _bleService.bluetoothDevices.forEach((element) {
      devices.add(InkWell(
        onTap: () {
          _waveglideDevice = WaveglideDevice(element.id.toString(), element.name);
          _preferences.setString("waveglideDevice",
            _waveglideDevice.toJsonString()
          );
          Navigator.pop(context, _waveglideDevice);
        },
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: <Widget>[
            Text(element.name,
                textAlign: TextAlign.left,
                style: TextStyle(
                    color: Colors.white
                )
            ),
            Text('[' + element.id.toString().substring(0, 18) + '1]',
                textAlign: TextAlign.left,
                style: TextStyle(
                  color: Colors.white,
                  fontSize: 10,
                )
            ),
            SizedBox(height: 20)
          ],
        )
      ));
    });
    return AlertDialog(
      title: Text("Select waveglide device"),

      content: _preferences == null ? Text('Loading...', style: TextStyle(color: Colors.white)) : Container(
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: devices,

        )
      ),
      actions: <Widget>[ FlatButton(
        child: Text("Cancel"),
        onPressed: () {
          Navigator.pop(context, _waveglideDevice);
        },
      ),
        FlatButton(
          child: Text("Reset"),
          onPressed: () {
            _preferences.remove("waveglideDevice");
            Navigator.pop(context, null);
          },
        ),
      ],
    );
  }
}