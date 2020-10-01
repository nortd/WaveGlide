class TimeUtil {

  static String secondsToHM(int seconds) {
    int hour = (seconds / 3600).floor();
    int minutes = ((seconds % 3600) / 60).round();

    return hour.toString() + ":" + twoDigits(minutes);
  }

  static String twoDigits(int n) {
    if (n >= 10) return "$n";
    return "0$n";
  }
}