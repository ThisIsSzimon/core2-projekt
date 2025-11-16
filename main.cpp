#include <hFramework.h>

void hMain()
{
    hExt.serial.init(115200);  // MUSI być ten sam baud co na ESP32
    hExt.serial.printf("Sterowanie LED1 przez hExt.serial (q/w/e)\r\n");
    hExt.serial.printf("q = ON, w = OFF, e = TOGGLE\r\n");

    while (true)
    {
        if (hExt.serial.available() > 0)
        {
            char c = hExt.serial.getch();
            hExt.serial.printf("KEY '%c' (%d)\r\n", c, (int)c);

            if (c == 'q') {
                hLED1.on();
                hExt.serial.printf("LED1: ON\r\n");
            }
            else if (c == 'w') {
                hLED1.off();
                hExt.serial.printf("LED1: OFF\r\n");
            }
            else if (c == 'e') {
                hLED1.toggle();
                sys.delay(500);
                hExt.serial.printf("LED1: TOGGLE\r\n");
            }
            else {
                hExt.serial.printf("Nieznany: '%c' (%d)\r\n", c, (int)c);
            }
        }

        sys.delay(10);
    }
}
