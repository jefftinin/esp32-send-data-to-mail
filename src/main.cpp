
#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#include <Ticker.h>
#include <string.h>

// Ticker smtpTicker;
Ticker serialTicker;

#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>)
#include <WiFiS3.h>
#endif

#include <ESP_Mail_Client.h>

#define WIFI_SSID "OnlyF1"
#define WIFI_PASSWORD "\"PZ9T]H*1&x&o)jR#0L,"

/** For Gmail, the app password will be used for log in
 *  Check out https://github.com/mobizt/ESP-Mail-Client#gmail-smtp-and-imap-required-app-passwords-to-sign-in
 *
 * For Yahoo mail, log in to your yahoo mail in web browser and generate app password by go to
 * https://login.yahoo.com/account/security/app-passwords/add/confirm?src=noSrc
 *
 * To use Gmai and Yahoo's App Password to sign in, define the AUTHOR_PASSWORD with your App Password
 * and AUTHOR_EMAIL with your account email.
 */

/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com */
#define SMTP_HOST "disroot.org"

/** The smtp port e.g.
 * 25  or esp_mail_smtp_port_25
 * 465 or esp_mail_smtp_port_465
 * 587 or esp_mail_smtp_port_587
 */
#define SMTP_PORT esp_mail_smtp_port_587

/* The log in credentials */
#define AUTHOR_EMAIL "octavi.ty@disroot.org"
#define AUTHOR_PASSWORD "qZMN+kAkLwffgrLPv-un5Q_qxoM2+PLpTW7SRtLvQ-_.FrHwjY9vPnxh.tiDv5.Y"

/* Recipient email address */
#define RECIPIENT_EMAIL "octavio.tiberion@gmail.com"

/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

volatile int index1;
volatile int index2;
volatile int index3;
String data;
String temp;
String tds;
String ec;
String ph;
String htmlMsg;
unsigned long lastExecTime = 0;
const unsigned long interval = 300000;

float tempThreshold = 35;
float ecThreshold = 2.0;
float phThreshold = 6.0;

/* Declare the message class */
SMTP_Message message;

volatile bool busy = false;

void serialProcess()
{
  if (Serial2.available() && !busy)
  {
    String data = Serial2.readStringUntil('\n');
    Serial.println("Received: " + data);
    // Split by commas
    index1 = data.indexOf(',');
    index2 = data.indexOf(',', index1 + 1);
    index3 = data.indexOf(',', index2 + 1);

    temp = data.substring(0, index1);
    tds = data.substring(index1 + 1, index2);
    ec = data.substring(index2 + 1, index3);
    ph = data.substring(index3 + 1);

    Serial.println("Temp: " + temp);
    Serial.println("TDS: " + tds);
    Serial.println("EC: " + ec);
    Serial.println("pH: " + ph);
  }
}

void smtpProcess()
{
  if (!busy)
  {
    busy = 1;
  }
  else
  {
    return;
  }
  smtp.debug(1);

  smtp.callback(smtpCallback);

  Session_Config config;

  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;

  config.login.user_domain = F("127.0.0.1");

  /*
  Set the NTP config time
  For times east of the Prime Meridian use 0-12
  For times west of the Prime Meridian add 12 to the offset.
  Ex. American/Denver GMT would be -6. 6 + 12 = 18
  See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
  */
  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 8;
  config.time.day_light_offset = 0;

  /* The full message sending logs can now save to file */
  /* Since v3.0.4, the sent logs stored in smtp.sendingResult will store only the latest message logs */
  // config.sentLogs.filename = "/path/to/log/file";
  // config.sentLogs.storage_type = esp_mail_file_storage_type_flash;

  /* Set the message headers */
  message.sender.name = F("ESP32");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("Unit Message");
  message.addRecipient(F("Admin"), RECIPIENT_EMAIL);

  if ((ph.toFloat() < phThreshold))
  {
    message.subject = F("Soil pH too low!");
  }
  else if (ec.toFloat() > ecThreshold)
  {
    message.subject = F("Soil Electroconductivity too High!");
  }
  else if (temp.toFloat() > tempThreshold)
  {
    message.subject = F("Temperature too high!");
  }
  else
  {
    message.subject = F("Normal Reading");
  }

  // String htmlMsg = "<p>This is the html text message.</p><p>The message was sent via ESP device.</p>";

  htmlMsg = "<h1 style='font-size:24px;'>Sensor Readings</h1><p><b style='font-size:16px;'>Temperature:</b> <span style='font-size:14px;'>" + String(temp) + "</span><br><b style='font-size:16px;'>TDS:</b> <span style='font-size:14px;'>" + String(tds) + "</span><br><b style='font-size:16px;'>EC:</b> <span style='font-size:14px;'>" + String(ec) + "</span><br><b style='font-size:16px;'>pH:</b> <span style='font-size:14px;'>" + String(ph) + "</span></p>";

  // htmlMsg = "<h1 style='font-size:24px;'>Sensor Readings</h1><p><b style='font-size:16px;'>Temperature:</b> <span style='font-size:14px;'>" + String(temp) + "</span><br><b style='font-size:16px;'>TDS:</b> <span style='font-size:14px;'>" + String(tds) + "</span><br><b style='font-size:16px;'>EC:</b> <span style='font-size:14px;'>" + String(ec) + "</span><br><b style='font-size:16px;'>pH:</b> <span style='font-size:14px;'>" + String(ph) + "</span></p>";

  message.html.content = htmlMsg;

  /** The html text message character set e.g.
   * us-ascii
   * utf-8
   * utf-7
   * The default value is utf-8
   */
  message.html.charSet = F("us-ascii");

  /** The content transfer encoding e.g.
   * enc_7bit or "7bit" (not encoded)
   * enc_qp or "quoted-printable" (encoded)
   * enc_base64 or "base64" (encoded)
   * enc_binary or "binary" (not encoded)
   * enc_8bit or "8bit" (not encoded)
   * The default value is "7bit"
   */
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  /** The message priority
   * esp_mail_smtp_priority_high or 1
   * esp_mail_smtp_priority_normal or 3
   * esp_mail_smtp_priority_low or 5
   * The default value is esp_mail_smtp_priority_low
   */
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;

  /** The Delivery Status Notifications e.g.
   * esp_mail_smtp_notify_never
   * esp_mail_smtp_notify_success
   * esp_mail_smtp_notify_failure
   * esp_mail_smtp_notify_delay
   * The default value is esp_mail_smtp_notify_never
   */
  // message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  /* Set the custom message header */
  message.addHeader(F("Message-ID: <abcde.fghij@gmail.com>"));

  /* Set the TCP response read timeout in seconds */
  // smtp.setTCPTimeout(10);

  /* Connect to the server */
  if (!smtp.connect(&config))
  {
    MailClient.printf("Connection error, Status Code: %d, Error Code: %d, Reason: %s\n", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn())
  {
    Serial.println("Error, Not yet logged in.");
  }
  else
  {
    if (smtp.isAuthenticated())
      Serial.println("Successfully logged in.");
    else
      Serial.println("Connected with no Auth.");
  }

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    MailClient.printf("Error, Status Code: %d, Error Code: %d, Reason: %s\n", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());

  // to clear sending result log
  // smtp.sendingResult.clear();

  MailClient.printf("Free Heap: %d\n", MailClient.getFreeHeap());
  busy = false;
}

void setup()
{
  data.reserve(200);
  temp.reserve(200);
  tds.reserve(200);
  ec.reserve(200);
  ph.reserve(200);
  htmlMsg.reserve(200);
  Serial.begin(9600);
  Serial2.begin(19200, SERIAL_8N1, 16, 17);
  // smtpTicker.attach_ms(5000, smtpProcess);
  serialTicker.attach_ms(1000, serialProcess);

#if defined(ARDUINO_ARCH_SAMD)
  while (!Serial)
    ;
#endif

  Serial.println();

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
  multi.addAP(WIFI_SSID, WIFI_PASSWORD);
  multi.run();
#else
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

  Serial.print("Connecting to Wi-Fi");

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
  unsigned long ms = millis();
#endif

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    if (millis() - ms > 10000)
      break;
#endif
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /*  Set the network reconnection option */
  MailClient.networkReconnect(true);

  // The WiFi credentials are required for Pico W
  // due to it does not have reconnect feature.
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
  MailClient.clearAP();
  MailClient.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif
}

void loop()
{
  unsigned long currentTime = millis();

  if ((ph.toFloat() < phThreshold || ec.toFloat() > ecThreshold || temp.toFloat() > tempThreshold))
  {
    smtpProcess();
  }
  else
  {
    if (currentTime - lastExecTime >= interval)
    {
      smtpProcess();
      lastExecTime = currentTime;
    }
  }
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status)
{
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success())
  {
    // MailClient.printf used in the examples is for format printing via debug Serial port
    // that works for all supported Arduino platform SDKs e.g. SAMD, ESP32 and ESP8266.
    // In ESP8266 and ESP32, you can use Serial.printf directly.

    Serial.println("----------------");
    MailClient.printf("Message sent success: %d\n", status.completedCount());
    MailClient.printf("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);

      // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
      // your device time was synched with NTP server.
      // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
      // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)

      MailClient.printf("Message No: %d\n", i + 1);
      MailClient.printf("Status: %s\n", result.completed ? "success" : "failed");
      MailClient.printf("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      MailClient.printf("Recipient: %s\n", result.recipients.c_str());
      MailClient.printf("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}
