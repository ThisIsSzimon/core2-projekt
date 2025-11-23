#include <hFramework.h>

const int VEH_BRIDGE_POWER  = 500;  // moc dla pojazdu na moście (hMot4)
const int VEH_DESK_POWER    = 500;  // moc dla pojazdu na biurku (hMot1)

void hMain()
{
    // UART na złączu hExt (tu wpina się ESP32)
    hExt.serial.init(115200);

    // Na start zatrzymujemy wszystkie silniki
    hMot1.setPower(0);
    hMot2.setPower(0);
    hMot3.setPower(0);
    hMot4.setPower(0);

    Serial.printf("CORE2: start, oczekuje komend z ESP32...\r\n");

    for (;;)
    {
        char cmd;

        if (hExt.serial.read(&cmd, 1, 10))
        {
            Serial.printf("CORE2: cmd = %c\r\n", cmd);

            switch (cmd)
            {
            // ================= MOST (hMot2 + hMot3) =================
            case 'A':   // Most: Góra
                hMot2.setPower(-900);
                hMot3.setPower(-1000);
                break;

            case 'a':   // Most: Dół
                hMot2.setPower(1000);
                hMot3.setPower(1000);
                break;

            case 'X':   // STOP mostu
                hMot2.setPower(0);
                hMot3.setPower(0);
                break;

            // =========== POJAZD NA MOŚCIE (hMot4) ===========
            case 'B':   // Przód
                hMot4.setPower(VEH_BRIDGE_POWER);
                break;

            case 'b':   // Tył
                hMot4.setPower(-VEH_BRIDGE_POWER);
                break;

            case 'Y':   // STOP pojazdu na moście
                hMot4.setPower(0);
                break;

            // ========== POJAZD NA BIURKU (hMot1) ============
            case 'C':   // Przód
                hMot1.setPower(VEH_DESK_POWER);
                break;

            case 'c':   // Tył
                hMot1.setPower(-VEH_DESK_POWER);
                break;

            case 'Z':   // STOP pojazdu na biurku
                hMot1.setPower(0);
                break;

            default:
                break;
            }
        }

        sys.delay(5);
    }
}
