import 'dart:async';

import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:waveglide/data/ble_status.dart';
import 'package:waveglide/data/data_item.dart';
import 'package:waveglide/screens/preferences/preference_screen.dart';
import 'package:waveglide/services/ble_service.dart';
import 'package:waveglide/util/toast_util.dart';
import 'package:waveglide/widgets/altitude_value_tile.dart';
import 'package:waveglide/widgets/battery_value_tile.dart';
import 'package:waveglide/widgets/breath_cycle_time_value_tile.dart';
import 'package:waveglide/widgets/breath_strength_percentage_value_tile.dart';
import 'package:waveglide/widgets/chart.dart';
import 'package:waveglide/widgets/heart_rate_value_tile.dart';
import 'package:waveglide/widgets/oxygen_pulse_percentage_value_tile.dart';
import 'package:waveglide/widgets/pulsoxy_signal_strength_value_tile.dart';
import 'package:waveglide/widgets/remaining_oxygen_value_tile.dart';
import 'package:waveglide/widgets/spo2_value_tile.dart';
import 'package:charts_flutter/flutter.dart' as charts;

import 'package:waveglide/widgets/value_tile.dart';
import 'dart:convert';
import 'dart:math';

class MainScreen extends StatefulWidget {
  @override
  _MainScreenState createState() => _MainScreenState();
}

class _MainScreenState extends State<MainScreen> {

  int _pulsoxySpO2;
  int _pulsoxySignalStrength;
  int _pulsoxyHeartrate;

  int _oxygenRemainingPercentage;
  int _oxygenRemainingTimeSeconds;
  int _oxygenRemainingLiters;



  int _batteryPercentage;
  int _oxygenBar;
  int _respirationPeriodMs;
  int _oxygenPulsePercentage;
  String _oxygenTime;
  String _batteryTime;
  int _altitudeFl;
  int _breathStrengthPercentage;
  int _oxygenSeconds;
  int _batterySeconds;
  int _status;
  BLEService _bleService;
  StreamController<dynamic> _bleDataStreamController = new StreamController.broadcast();
  BleStatus _bleStatus;
  SharedPreferences _preferences;

  List<LinearSales> _spo2Values = [];
  List<LinearSales> _heartRateValues = [];
  List<LinearSales> _o2PulseValues = [];

  @override
  void initState() {
    super.initState();
    // Immediately get the state of FlutterBlue

    _bleService = BLEService();
    _bleService.status().last.then((value) => _bleStatus = value);

    _bleService.status().listen((status) {
      setState(() {
        _bleStatus = status;
        print(status);
      });
    });
    _bleService.data.listen((dataItem) {
      _bleDataStreamController.add(dataItem);
      if(dataItem.type == DataItemType.pulsoxySpO2) {
        setState(() {
          _pulsoxySpO2 = dataItem.value.toInt();

          _spo2Values.add(LinearSales(_spo2Values.length, _pulsoxySpO2));
        });
      } else if(dataItem.type == DataItemType.flightLevel) {
        setState(() {
          _altitudeFl = dataItem.value.toInt();
        });
      } else if(dataItem.type == DataItemType.batteryPercentage) {
        setState(() {
          _batteryPercentage = dataItem.value.toInt();
        });
      } else if(dataItem.type == DataItemType.pulsoxyHeartrate) {
        setState(() {
          _pulsoxyHeartrate = dataItem.value.toInt();
          _heartRateValues.add(LinearSales(_heartRateValues.length, _pulsoxyHeartrate));
        });
      } else if(dataItem.type == DataItemType.pulsoxySignalStrength) {
        setState(() {
          _pulsoxySignalStrength = dataItem.value.toInt();
        });
      } else if(dataItem.type == DataItemType.respirationPeriodMs) {
        setState(() {
          _respirationPeriodMs = dataItem.value.toInt();
        });
      } else if(dataItem.type == DataItemType.oxygenPercentage) {
        setState(() {
          _oxygenPulsePercentage = dataItem.value.toInt();
          _o2PulseValues.add(LinearSales(_o2PulseValues.length, _oxygenPulsePercentage));
        });
      } else if(dataItem.type == DataItemType.breathStrengthPercentage) {
        setState(() {
          _breathStrengthPercentage = dataItem.value.toInt();
        });
      } else if(dataItem.type == DataItemType.status) {
        setState(() {
          _status = dataItem.value.toInt();
        });
      }
    });

    _getData();
  }

  void _getData() async {
    _preferences = await SharedPreferences.getInstance();
  }

  Widget _getBluetoothMenuButton() {
    Icon icon = Icon(Icons.bluetooth_disabled,
      color: Colors.red,
    );
    if(_bleStatus == BleStatus.bluetoothStateOn) {
      icon = Icon(Icons.bluetooth,
        color: Colors.green,
      );
    } else if(_bleStatus == BleStatus.deviceScanning) {
      icon = Icon(Icons.bluetooth_searching,
        color: Colors.orange,
      );
    } else if(_bleStatus == BleStatus.deviceConnected) {
      icon = Icon(Icons.bluetooth_connected,
        color: Colors.green,
      );
    } else if(_bleStatus == BleStatus.deviceDisconnected) {
      icon = Icon(Icons.bluetooth,
        color: Colors.orange,
      );
    }
    return PopupMenuButton<int>(
      icon: icon,
      onSelected: (selectedIndex) {
        if(selectedIndex == 0) {
          _bleService.startScan();
        } else {
          _bleService.disconnect(reset: false);
        }
      },
      itemBuilder: (context) => [
        PopupMenuItem(
          value: 0,
          child: Text("Connect"),
        ),
        PopupMenuItem(
          value: 1,
          child: Text("Disconnect"),
        ),
      ],
    );
  }



  Choice _selectedChoice = preferencesChoices[0]; // The app's "state".

  void _select(Choice choice) {
    // Causes the app to rebuild with the new _selectedChoice.
    /*setState(() {
      _selectedChoice = choice;
    });*/
    Navigator.pushNamed(context, '/preferences');
  }

  String _setTileValue(Object value) {
    if(value == null) {
      return '-';
    }

    return value.toString();
  }

  List<charts.Series<LinearSales, int>> _createChartData() {

    return [
      new charts.Series<LinearSales, int>(
        id: 'SpO2 [%]',
        colorFn: (_, __) => charts.MaterialPalette.blue.shadeDefault,
        domainFn: (LinearSales sales, _) => sales.year,
        measureFn: (LinearSales sales, _) => sales.sales,
        data: _spo2Values,
      ),
      new charts.Series<LinearSales, int>(
        id: 'O2 Pulse [%]',
        colorFn: (_, __) => charts.MaterialPalette.green.shadeDefault,
        dashPatternFn: (_, __) => [2, 2],
        domainFn: (LinearSales sales, _) => sales.year,
        measureFn: (LinearSales sales, _) => sales.sales,
        data: _o2PulseValues,
      ),
      new charts.Series<LinearSales, int>(
        id: 'HR [b/m]',
        colorFn: (_, __) => charts.MaterialPalette.red.shadeDefault,

        domainFn: (LinearSales sales, _) => sales.year,
        measureFn: (LinearSales sales, _) => sales.sales,
        data: _heartRateValues,
      )..setAttribute(charts.measureAxisIdKey, 'secondaryMeasureAxisId'),

    ];
  }

  @override
  Widget build(BuildContext context) {

    List<Widget> tiles = <Widget>[];

    double size = (MediaQuery.of(context).size.width - 16) / 3;

    if(_preferences != null && _preferences.containsKey("waveglideDevice")) {
      tiles.add(Text("Connected with " + _bleService.getConnectedDeviceName(),
        style: TextStyle(
          color: Colors.white,
        ),
      )
      );
    } else {
      tiles.add(Text("Select your device in the app settings",
        style: TextStyle(
          color: Colors.red,
        ),
      )
      );
    }


    tiles.add(Row(
        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
        children: <Widget>[
          SpO2ValueTile(_pulsoxySpO2, size),
          HeartRateValueTile(_pulsoxyHeartrate, size),
          PulsoxySignalStrengthValueTile(_pulsoxySignalStrength, size),
        ]
    ));

    tiles.add(Row(
      mainAxisAlignment: MainAxisAlignment.center,
      children: <Widget>[
        OxygenPulsePercentageValueTile(_oxygenPulsePercentage, size),
        BreathStrengthPercentageValueTile(_breathStrengthPercentage, size),
        BreathCycleTimeValueTile(_respirationPeriodMs, size),
      ],
    ));


    tiles.add(Row(
      mainAxisAlignment: MainAxisAlignment.center,
      children: <Widget>[
        RemainingOxygenValueTile(_bleDataStreamController.stream, size),
        AltitudeValueTile(_bleDataStreamController.stream, size),
        BatteryValueTile(_bleDataStreamController.stream, size),
      ],
    ));

    tiles.add(
      Container(
        height: 200,
        child: DashPatternLineChart(
          _createChartData(),
          animate: false,
        )
      )

    );


    return Scaffold(
      appBar: AppBar(
        title: Text("Waveglide"),
        actions: <Widget>[
          // action button
          _getBluetoothMenuButton(),
          // overflow menu
          PopupMenuButton<Choice>(
            onSelected: _select,
            itemBuilder: (BuildContext context) {
              return preferencesChoices.map((Choice choice) {
                return PopupMenuItem<Choice>(
                  value: choice,
                  child: Text(choice.title),
                );
              }).toList();
            },
          ),
        ],
      ),
      body: SingleChildScrollView(
        child:Container(
          margin: EdgeInsets.all(8),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.start,
            children: tiles,
          )
        ),
      )
    );
  }

  void _updateValues() {
    //sendValues([1,2,3]);
    if(_bleService.isConnected()) {
      _bleService.disconnect(reset: false);
      setState(() {

      });
    } else {
      _bleService.startScan();
      setState(() {

      });
    }

  }
}

class Choice {
  const Choice({this.title, this.icon});

  final String title;
  final IconData icon;
}

const List<Choice> preferencesChoices = const <Choice>[
  const Choice(title: 'Settings', icon: Icons.settings),
  const Choice(title: 'About', icon: Icons.info_outline),
];

const List<Choice> bluetoothChoices = const <Choice>[
  const Choice(title: 'Connect', icon: Icons.bluetooth_connected),
  const Choice(title: 'Disconnect', icon: Icons.bluetooth_disabled),
];