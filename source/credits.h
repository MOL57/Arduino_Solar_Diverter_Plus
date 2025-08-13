/*
=====================================================================
credits.h
gets and prints the compilation credits (file name, date and time)
=====================================================================
*/

const int MAX_DATE_TIME_LENGTH = 20;            // maximum length of the compilation date+time string

class Credits
{
  public:
    Credits(void) {};
    void begin(char *, char *, char *);
    void getFileName(char *);
    void getDateTime(char *, char *);
    void print(void);
    char *fileName = "NO NAME";
    char fileDateTime[MAX_DATE_TIME_LENGTH+1];
};

void Credits::begin(char *filePath_arg, char *fileDate_arg, char *fileTime_arg)   // gets and formats the file name and date-time of compilation
{
  getFileName(filePath_arg);
  getDateTime(fileDate_arg, fileTime_arg);
  print();
}

void Credits::getFileName(char *filePath_arg)
{
  int i;
  int n;
  
  n = strlen(filePath_arg);

  for( i=n-1; i>=0; i-- )
    if(filePath_arg[i] == '\\')
    {
      fileName = filePath_arg + (++i);
      break;
    }

  //Serial.println(fileName);
}

void Credits::getDateTime(char *fileDate_arg, char *fileTime_arg)
{

  int month, day, year;
  static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char localBuffer[4];

  sscanf(fileDate_arg, "%3s %d %d", localBuffer, &day, &year);
  month = ( strstr(month_names, localBuffer) - month_names ) / 3 + 1;
  snprintf(fileDateTime, MAX_DATE_TIME_LENGTH, "%d/%02d/%02d %s", year, month, day, fileTime_arg);

  //Serial.println(fileDateTime);
}

void Credits::print(void)           // prints the credits onto the serial
{
  Serial.println("\n\n\n\n");
  Serial.println("===============================");
  Serial.println(fileName);
  Serial.println(fileDateTime);
  Serial.println("===============================");
  Serial.println("");
}
