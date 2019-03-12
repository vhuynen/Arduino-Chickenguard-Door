#include <Timezone.h>

String getLiteralDate(String timestr) {
  char timestamp[sizeof(timestr)];
  timestr.toCharArray(timestamp, sizeof(timestr));

  String date = "";
  time_t t = (time_t) strtol(timestamp, NULL, 10);
  date += ((day(t) < 10) ? "0" : "");
  date += day(t);
  date += " ";
  date += monthShortStr(month(t));
  date += " ";
  date += year(t);
  date += " ";
  date += ((hour(t) < 10) ? "0" : "");
  date += hour(t);
  date += ":";
  date += ((minute(t) < 10) ? "0" : "");
  date += minute(t);
  date += ":";
  date += ((second(t) < 10) ? "0" : "");
  date += second(t);

  return date;
}
