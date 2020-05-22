/**
 * This sketch demonstrates reliably configuraing the Adafruit Ultimate GPS Breakout.
 *
 * This sketch will wait for MTK3339 startup messages before sending configuration messages. After a configuration
 * message is sent, the program will wait for MTK3339 ACK messages.
 * 
 * More details on MTK3339 commands can be found here: https://learn.adafruit.com/adafruit-ultimate-gps/downloads
 * 
 * Wiring:
 *      - RX/TX on Serial 1
 *      - Pin 7 to GPS "EN" input (can be changed with GPS_ENABLE constant).
 */
#include <string.h>

using size_type = unsigned;
using time_type = unsigned long;

#define GpsSerial Serial1

constexpr auto GPS_ENABLE = 7; // Output from Arduino, input to "EN" on GPS breakout.
constexpr time_type GPS_SETUP_TIMEOUT_MS = 1000; // Wait time for startup and configuration messages.

void setup()
{
    Serial.begin(57600);

    // Set GPS pin to low to power off. This will full restart on reset button press (or Serial monitor open on
    // some boards).
    Serial.println(F("Turning off GPS_ENABLE."));
    pinMode(GPS_ENABLE, OUTPUT);
    digitalWrite(GPS_ENABLE, LOW);

    mtk3339_setup();
}

void loop()
{
    // Mirror serial from GPS to USB.
    if(GpsSerial.available())
    {
        int c = GpsSerial.read();
        Serial.write(c);
    }
}

/**
 * Set up the MTK3339 GPS module. This will wait for system startup messages and send configuration messages.
 * 
 * Nothing is done on failure, even though it should probably try to send a configuration message until it is
 * successful.
 * 
 * This method blocks until setup and ACK messages are received, or until a timeout. This will take a noticeable
 * amount of time to execute at 16 MHz.
 */
void mtk3339_setup()
{
    static const char mtk314_ack[] = "$PMTK001,314,3*36";
    static const char mtk220_ack[] = "$PMTK001,220,3*30";

    bool setup_rc;

    Serial.println(F("Configuring MTK3339..."));

    GpsSerial.begin(9600);

    // Write 1 to the GPS Enable pin, powring up the GPS
    Serial.println(F("Turning on GPS_ENABLE."));
    delay(100);
    digitalWrite(GPS_ENABLE, HIGH);

    // Wait for the special startup messages.
    Serial.println(F("Waiting for startup messages..."));
    setup_rc = mtk3339_wait_for_startup_messages(GPS_SETUP_TIMEOUT_MS);
    if(!setup_rc)
    {
        Serial.println(F("ERROR: Failed to get expected MTK3339 power on messages!"));
    }
    else
    {
        Serial.println(F("Got MTK3339 startup messages."));
    }

    // Configure to send GGA only.    
    Serial.println(F("Sending 314 (message type config)..."));
    GpsSerial.println(F("$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"));
    setup_rc = mtk3339_wait_for_exact_msg(mtk314_ack, GPS_SETUP_TIMEOUT_MS);
    if(!setup_rc)
    {
        Serial.println(F("ERROR: Did not find 314 ACK!"));
    }
    else
    {
        Serial.println(F("Found 314 successful ACK."));
    }
    
    // Configure to send at a 5 second interval.
    Serial.println(F("Sending 220 (message speed config)..."));
    GpsSerial.println(F("$PMTK220,5000*1B"));
    setup_rc = mtk3339_wait_for_exact_msg(mtk220_ack, GPS_SETUP_TIMEOUT_MS);
    if(!setup_rc)
    {
        Serial.println(F("ERROR: Did not find 220 ACK!"));
    }
    else
    {
        Serial.println(F("Found 220 successful ACK."));
    }

    Serial.println(F("Setup complete."));
}

/**
 * Collect a message in the buffer up to the given size. Result will always be null terminated. This will not collect
 * newline or carriage return characters.
 * 
 * This is a blocking method.
 */
void collect_message(char* buffer, size_type buffer_size, time_type timeout)
{
    size_type idx = 0;
    time_type start = millis();
    time_type elapsed = 0;

    while(elapsed < timeout)
    {
        if(GpsSerial.available())
        {
            char c = GpsSerial.read();

            // Don't collect newline or carriage returns to make string comparisons easier.
            if(c != '\n' && c != '\r' && idx < buffer_size)
            {
                buffer[idx] = c;
                ++idx;
            }

            // A NMEA message stops at a newline, so break out of the process loop here.
            if(c == '\n')
            {                
                break;
            }
        }

        elapsed = millis() - start;
    }

    // Always null terminate output.
    if(idx < buffer_size)
    {
        buffer[idx] = 0;
    }
    else
    {
        buffer[buffer_size - 1] = 0;
    }
}

/**
 * Wait for the two special messages that are sent on GPS startup. There are also some other messages that get sent
 * and are not in the spec, so wait for the specific message strings.
 * 
 * This is a blocking method.
 */
bool mtk3339_wait_for_startup_messages(time_type timeout)
{
    static const char mtk_010[] = "$PMTK010,001*2E";
    static const char mtk_011[] = "$PMTK011,MTKGPS*08";

    char buffer[32];
    int found_flags = 0;

    time_type start = millis();
    time_type elapsed = 0;

    while(elapsed < timeout)
    {
        // Blocking collect of messages.
        collect_message(buffer, sizeof(buffer), timeout);

        if(strcmp(buffer, mtk_010) == 0)
        {
            Serial.println(F("Found startup message 001."));
            found_flags |= 1;
        }
        else if(strcmp(buffer, mtk_011) == 0)
        {
            Serial.println(F("Found startup MTKGPS text message."));
            found_flags |= 2;
        }
        else
        {
            Serial.print(F("Found other message: \""));
            Serial.print(buffer);
            Serial.println(F("\"."));
        }
        
        if(found_flags == 3)
        {
            return true;
        }

        elapsed = millis() - start;
    }

    Serial.println(F("Timeout when waiting for startup messages."));

    return false;
}

/**
 * Wait for an exact message from the GPS serial. If a message is not found within the timeout period, false is
 * returned.
 * 
 * This is a blocking method.
 */
bool mtk3339_wait_for_exact_msg(const char* msg, time_type timeout)
{
    char buffer[32];
    time_type start = millis();
    time_type elapsed = 0;

    while(elapsed < timeout)
    {
        collect_message(buffer, sizeof(buffer), timeout);

        if(strcmp(buffer, msg) == 0)
        {
            return true;
        }
        else
        {
            // For debugging, output the other messages received while waiting for the ACK.
            Serial.print(F("Found when waiting for ACK: "));
            Serial.print(buffer);
            Serial.println();
        }

        elapsed = millis() - start;
    }

    Serial.println(F("Timeout when waiting for exact message."));

    return false;
}
