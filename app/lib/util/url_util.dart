import 'package:url_launcher/url_launcher.dart';

class UrlUtil {
  static void launchURL(String url) async {
    if (await canLaunch(url)) {
      await launch(url);
    } else {
      throw 'Could not launch $url';
    }
  }

  static void launchTel(String url) async {
    url = url.replaceAll(' ', '');
    if (await canLaunch(url)) {
      await launch(url);
    } else {
      throw 'Could not launch $url';
    }
  }
}
