import 'package:flutter/material.dart';
import 'package:waveglide/screens/preferences/preference_screen.dart';
import 'package:waveglide/screens/main/main_screen.dart';

class Routes {

  static Map<String, WidgetBuilder> list() {

    return {
      '/main': (BuildContext context) => MainScreen(),
      '/preferences': (BuildContext context) => PreferenceScreen(),
    };
  }

}