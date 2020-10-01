import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:waveglide/data/waveglide_device.dart';
import 'package:waveglide/widgets/bluetooth_devices_dialog.dart';
import 'package:waveglide/widgets/oxygen_pressure_dialog.dart';

class PreferenceScreen extends StatefulWidget {
  @override
  State<StatefulWidget> createState() => _PreferenceScreenState();
}

class _PreferenceScreenState extends State<PreferenceScreen> {

  WaveglideDevice _waveglideDevice;
  bool _warnOnLowSpO2;
  bool _warnOnLowBattery;
  bool _warnOnLowOxygen;
  double _oxygenBottleVolume;
  double _oxygenBottleRemainingPressure = 120;
  List<double> oxygenBottleVolumeList = [0.8, 1, 2, 3];
  SharedPreferences _preferences;

  @override
  void initState() {
    super.initState();
    _getData();
  }

  void _getData() async {
    _preferences = await SharedPreferences.getInstance();

    if(_preferences.containsKey("waveglideDevice")) {
      _waveglideDevice = WaveglideDevice.fromJsonString(_preferences.getString("waveglideDevice")) ;
    }

    _warnOnLowSpO2 = _preferences.getBool("warnOnLowSpO2") ?? true;
    _warnOnLowBattery = _preferences.getBool("warnOnLowBattery") ?? true;
    _warnOnLowOxygen = _preferences.getBool("warnOnLowOxygen") ?? true;
    _oxygenBottleVolume = _preferences.getDouble("oxygenBottleVolume") ?? 2;
    _oxygenBottleRemainingPressure = _preferences.getDouble("oxygenBottleRemainingPressure") ?? 200;

    setState((){});
  }

  void onOxygenBottleVolumeChanged(double value) {


    setState(() {
      _oxygenBottleVolume =  value;
    });
  }

  void _showAlertDialog() async {

    _oxygenBottleRemainingPressure = await showDialog<double>(
      context: context,
      builder: (context) => OxygenPressureDialog(),
    );

    setState(() {

    });
  }

  void _showWaveglideDeviceDialog() async {

    _waveglideDevice = await showDialog<WaveglideDevice>(
      context: context,
      builder: (context) => BluetoothDevicesDialog(),
    );



    setState(() {

    });
  }

  @override
  Widget build(BuildContext context) {

    List<PopupMenuItem> oxygenBottleVolumeItems = [];
    for(double oxygenBottleVolume in oxygenBottleVolumeList) {
      oxygenBottleVolumeItems.add(PopupMenuItem<double>(
        child: new Text(oxygenBottleVolume.toString() + "L"),
        value: oxygenBottleVolume
      ));
    }


    return Container(
      child: Scaffold(
        appBar: AppBar(
          // Here we take the value from the MyHomePage object that was created by
          // the App.build method, and use it to set our appbar title.
          title: Text("Settings"),
        ),
        body: Center(
          child: _preferences == null ? Text('Loading...', style: TextStyle(color: Colors.white)) : Column(
            children: <Widget>[
              InkWell(
                  onTap: () {
                    _showWaveglideDeviceDialog();
                  },
                  child: ListTile(
                      title: Text("Waveglide device"),
                      subtitle: Text(_waveglideDevice != null ? _waveglideDevice.deviceName : "-"),
                      trailing: Padding(
                          padding: EdgeInsets.fromLTRB(0, 0, 12, 0),
                          child: Icon(Icons.more_horiz)
                      )
                  )
              ),
              CheckboxListTile(
                title: Text("Warn on low SpO2"),
                value: _warnOnLowSpO2,
                onChanged: (value) {
                  setState(() {
                    _warnOnLowSpO2 = value;
                    _preferences.setBool('warnOnLowSpO2', _warnOnLowSpO2);
                  });
                },
              ),
              CheckboxListTile(
                title: Text("Warn on low battery"),
                value: _warnOnLowBattery,
                onChanged: (value) {
                  setState(() {
                    _warnOnLowBattery = value;
                    _preferences.setBool('warnOnLowBattery', _warnOnLowBattery);
                  });
                },
              ),
              CheckboxListTile(
                title: Text("Warn on low oxygen"),
                value: _warnOnLowOxygen,
                onChanged: (value) {
                  setState(() {
                    _warnOnLowOxygen = value;
                    _preferences.setBool('warnOnLowOxygen', _warnOnLowOxygen);
                  });
                },
              ),
              ListTile(
                title: Text("O2 bottle volume [L]"),
                subtitle: Text(_oxygenBottleVolume.toString() + "L"),
                trailing: PopupMenuButton(
                  icon: null,
                  onSelected: (selectedDropDownItem) => onOxygenBottleVolumeChanged(selectedDropDownItem),
                  itemBuilder: (BuildContext context) => oxygenBottleVolumeItems,
                  tooltip: "Tap to select oxygen bottle volume.",
                ),
                onTap: (){

                },
              ),
              InkWell(
                onTap: () {
                  _showAlertDialog();
                },
                child: ListTile(
                  title: Text("O2 bottle current pressure [bar]"),
                  subtitle: Text(_oxygenBottleRemainingPressure.round().toString() + " bar"),
                  trailing: Padding(
                    padding: EdgeInsets.fromLTRB(0, 0, 12, 0),
                    child: Icon(Icons.more_horiz)
                  )
                )
              )
            ],
          )
        ),
      )
    );
  }
}