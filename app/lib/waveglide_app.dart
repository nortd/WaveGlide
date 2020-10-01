import 'package:waveglide/routes.dart';
import 'package:flutter/material.dart';
import 'package:waveglide/screens/main/main_screen.dart';
import 'package:waveglide/screens/preferences/preference_screen.dart';

class WaveglideApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Waveglide',
      theme: ThemeData.dark(),
      routes: Routes.list(),
      home: MainScreen(),
    );
  }
}

