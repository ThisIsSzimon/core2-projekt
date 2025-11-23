#include <hFramework.h>
#include <DistanceSensor.h>

using namespace hModules;

// ----------------- Konfiguracja -----------------
const int ROBOT_SPEED = 400;                 // moc dla hMot1 (0–1000)
const int CANYON_DIST_THRESHOLD = 250;       // [mm] powyżej tej odległości uznajemy, że czujnik widzi "dziurę"

// hMot2 + hMot3 – most + podnoszenie/opuszczanie robota
const int BRIDGE_DOWN_POWER        = -900;   // kierunek: most w dół + robot w górę (dobierz znak)
const int BRIDGE_UP_POWER          = 900;    // przeciwny kierunek: most w górę + robot w dół

const uint32_t BRIDGE_ALIGN_STABLE_MS       = 500;   // jak długo czujniki 1 i 4 muszą widzieć biurko, zanim ruszą hMot2, hMot3
const uint32_t BRIDGE_DOWN_ROBOT_UP_TIME_MS = 3000;  // czas: most w dół + robot w górę
const uint32_t BRIDGE_UP_ROBOT_DOWN_TIME_MS = 2500;  // czas: most w górę + robot w dół

// hMot4 – przejazd po moście
const int H4_ROT_TICKS      = 2000;   // ile ticków po moście – do kalibracji
const int H4_ROT_SPEED      = 300;    // „moc” hMot4 (0–1000)
const uint32_t WAIT_AFTER_H4_MS = 500; // przerwa po przejechaniu po moście

// ----------------- Stany automatu -----------------
enum class State {
    WAIT_START,             // czekamy na wciśnięcie hBtn1
    SEARCH_CANYON,          // jedziemy do przodu, aż zobaczymy kanion (czujnik 3)
    WAIT_BRIDGE_ALIGN,      // czekamy aż czujnik 1 i 4 są nad biurkiem (stabilnie)
    BRIDGE_DOWN_ROBOT_UP,   // hMot2 i hMot3 opuszczają most + podnoszą robota (czasowo)
    MOVE_ROBOT_ACROSS,      // hMot4 przewozi robota po moście
    BRIDGE_UP_ROBOT_DOWN,   // hMot2 i hMot3 podnoszą most + opuszczają robota (czasowo)
    MOVE_BRIDGE_AHEAD       // hMot4 wraca o tyle samo, żeby most znów był przed robotem
};

// ----------------- Funkcje pomocnicze -----------------

// true jeśli kanion
bool isCanyon(int dist)
{
    if (dist == 0) {
        // często 0 oznacza: brak echa / bardzo daleko
        return true;
    }

    if (dist > CANYON_DIST_THRESHOLD) {
        // to trzeba skalibrować - CANYON_DIST_THRESHOLD
        return true;
    }

    return false;
}

// true jeśli biurko
bool isDesk(int dist)
{
    return !isCanyon(dist);
}

void hMain()
{
    // Czujniki:
    DistanceSensor sens1(hSens1); // 1: koniec mostu (nad biurkiem 2)
    DistanceSensor sens2(hSens2); // 2: koniec robota
    DistanceSensor sens3(hSens3); // 3: początek robota (wykrywanie początku kanionu)
    DistanceSensor sens4(hSens4); // 4: początek mostu (nad biurkiem 1)

    // Polaryzacja silnika:
    hMot4.setEncoderPolarity(Polarity::Reversed);
    hMot4.setMotorPolarity(Polarity::Normal);

    // Start – zatrzymujemy wszystko
    hMot1.setPower(0);
    hMot2.setPower(0);
    hMot3.setPower(0);
    hMot4.setPower(0);

    State state = State::WAIT_START;   // <<< nowy stan startowy

    uint64_t stateEntryTime = sys.getRefTime();
    uint64_t alignStartTime = 0;  // kiedy d1 i d4 jednocześnie wykryły biurko
    bool h4MoveStarted = false;   // używane w stanach MOVE_ROBOT_ACROSS i MOVE_BRIDGE_AHEAD

    Serial.printf("START: automat kanionu z cyklem wielostolowym.\r\n");
    Serial.printf("Wcisnij hBtn1 aby wystartowac automat.\r\n");

    for (;;)
    {
        int d1 = sens1.getDistance();  // czujnik 1 (koniec mostu)
        int d2 = sens2.getDistance();  // czujnik 2 (koniec robota)
        int d3 = sens3.getDistance();  // czujnik 3 (poczatek robota)
        int d4 = sens4.getDistance();  // czujnik 4 (poczatek mostu)

        uint64_t now = sys.getRefTime();

        Serial.printf("d1=%d d2=%d d3=%d d4=%d  state=%d\r\n",
                      d1, d2, d3, d4, (int)state);

        switch (state)
        {
        case State::WAIT_START:
            // wszystko stoi, czekamy na przycisk
            hMot1.setPower(0);
            hMot2.setPower(0);
            hMot3.setPower(0);
            hMot4.setPower(0);

            if (hBtn1.isPressed()) {
                Serial.printf("hBtn1 pressed -> START automatu (SEARCH_CANYON)\r\n");
                state = State::SEARCH_CANYON;
                stateEntryTime = now;

                // prosty debounce – poczekaj aż puści przycisk
                while (hBtn1.isPressed()) {
                    sys.delay(20);
                }
            }
            break;

        case State::SEARCH_CANYON:
            // --- KROK 1: jedziemy do przodu, aż czujnik 3 zobaczy kanion ---
            hMot1.setPower(ROBOT_SPEED);   // jeśli jedzie w złą stronę -> zmień na -ROBOT_SPEED

            if (isCanyon(d3)) {
                hMot1.setPower(0);
                Serial.printf("KROK 1: WYKRYTO KANION -> STOP ROBOT\r\n");

                state = State::WAIT_BRIDGE_ALIGN;
                stateEntryTime = now;
                alignStartTime = 0;
                Serial.printf("KROK 1: Przejscie do WAIT_BRIDGE_ALIGN\r\n");
            }
            break;

        case State::WAIT_BRIDGE_ALIGN:
        {
            // --- KROK 2: czekamy, aż cz.1 i cz.4 przez pewien czas będą nad biurkiem ---
            hMot1.setPower(0);
            hMot2.setPower(0);
            hMot3.setPower(0);
            hMot4.setPower(0);

            bool s1Desk = isDesk(d1);
            bool s4Desk = isDesk(d4);

            if (s1Desk && s4Desk) {
                if (alignStartTime == 0) {
                    alignStartTime = now;
                    Serial.printf("KROK 2: Bridge align condition START (d1 & d4 nad biurkiem)\r\n");
                } else if (now - alignStartTime >= BRIDGE_ALIGN_STABLE_MS) {
                    Serial.printf("KROK 2: Bridge align STABLE -> start BRIDGE_DOWN_ROBOT_UP\r\n");
                    state = State::BRIDGE_DOWN_ROBOT_UP;
                    stateEntryTime = now;
                }
            } else {
                if (alignStartTime != 0) {
                    Serial.printf("KROK 2: Bridge align LOST, reset timer\r\n");
                }
                alignStartTime = 0;
            }
            break;
        }

        case State::BRIDGE_DOWN_ROBOT_UP:
        {
            // --- KROK 3: most w dół + robot w górę (czasowo) ---
            uint64_t elapsed = now - stateEntryTime;

            hMot2.setPower(BRIDGE_DOWN_POWER);
            hMot3.setPower(BRIDGE_DOWN_POWER);

            if (elapsed >= BRIDGE_DOWN_ROBOT_UP_TIME_MS) {
                hMot2.setPower(0);
                hMot3.setPower(0);
                Serial.printf("KROK 3: BRIDGE_DOWN_ROBOT_UP done -> MOVE_ROBOT_ACROSS\r\n");

                state = State::MOVE_ROBOT_ACROSS;
                stateEntryTime = now;
                h4MoveStarted = false;
            }
            break;
        }

        case State::MOVE_ROBOT_ACROSS:
        {
            // --- KROK 4: przejazd robota po moście (hMot4.rotRel) ---
            if (!h4MoveStarted) {
                h4MoveStarted = true;
                Serial.printf("KROK 4: MOVE_ROBOT_ACROSS: start hMot4.rotRel(+ticks)\r\n");

                // RUCH BLOKUJĄCY – robot jedzie po moście
                hMot4.rotRel(H4_ROT_TICKS, H4_ROT_SPEED, true, INFINITE);

                stateEntryTime = sys.getRefTime();
                Serial.printf("KROK 4: MOVE_ROBOT_ACROSS: finished, czekam na sens2 & sens3 desk\r\n");
            } else {
                bool s2Desk = isDesk(d2);
                bool s3Desk = isDesk(d3);

                if ((now - stateEntryTime >= WAIT_AFTER_H4_MS) && s2Desk && s3Desk) {
                    Serial.printf("KROK 4: Robot na biurku (sens2 & sens3 desk) -> BRIDGE_UP_ROBOT_DOWN\r\n");
                    state = State::BRIDGE_UP_ROBOT_DOWN;
                    stateEntryTime = now;
                }
            }
            break;
        }

        case State::BRIDGE_UP_ROBOT_DOWN:
        {
            // --- KROK 5: most w górę + robot w dół (czasowo, przeciwny kierunek) ---
            uint64_t elapsed = now - stateEntryTime;

            hMot2.setPower(BRIDGE_UP_POWER);
            hMot3.setPower(BRIDGE_UP_POWER);

            if (elapsed >= BRIDGE_UP_ROBOT_DOWN_TIME_MS) {
                hMot2.setPower(0);
                hMot3.setPower(0);
                Serial.printf("KROK 5: BRIDGE_UP_ROBOT_DOWN done -> MOVE_BRIDGE_AHEAD\r\n");

                state = State::MOVE_BRIDGE_AHEAD;
                stateEntryTime = now;
                h4MoveStarted = false;  // użyjemy ponownie dla ruchu mostu do przodu
            }
            break;
        }

        case State::MOVE_BRIDGE_AHEAD:
        {
            // --- KROK 6: hMot4 ma przejechać tyle samo, ale w przeciwnym kierunku,
            //             aby wystawić most przed robota na nowym biurku ---
            if (!h4MoveStarted) {
                h4MoveStarted = true;
                Serial.printf("KROK 6: MOVE_BRIDGE_AHEAD: start hMot4.rotRel(-ticks)\r\n");

                // RUCH BLOKUJĄCY – most jedzie do przodu względem robota
                hMot4.rotRel(-H4_ROT_TICKS, H4_ROT_SPEED, true, INFINITE);

                Serial.printf("KROK 6: MOVE_BRIDGE_AHEAD: finished -> wracamy do SEARCH_CANYON na kolejnym stole\r\n");

                // Przygotowanie do kolejnego cyklu na nowym biurku:
                hMot1.setPower(0);
                hMot2.setPower(0);
                hMot3.setPower(0);
                hMot4.setPower(0);

                alignStartTime = 0;
                state = State::SEARCH_CANYON;
                stateEntryTime = sys.getRefTime();
            }
            break;
        }
        }

        sys.delay(50);
    }
}
