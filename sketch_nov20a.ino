// Chromeye — NN classifier + 3-cycle majority voting
// Uses 1 LDR on A0 (INPUT_PULLUP wiring), LEDs on 9/10/11
// Prints raw ADC, normalized reflectance and per-cycle + batch detection

const int LDR_PIN = A0;
const int RED_PIN = 9;
const int GREEN_PIN = 10;
const int BLUE_PIN = 11;

const int LED_BRIGHT = 120;    // same as before
const int LED_ON_MS   = 900;   // 0.9s per LED
const int SAMPLES     = 8;

const int CYCLES = 3;          // cycles per batch
const unsigned long PAUSE_MS = 15000UL; // 15s between batches

// Prototypes from your measured averaged data (normalized reflectance vectors)
// Order: Black, White, Green, Red, Yellow, DarkSkyBlue
const int N_PROT = 6;
const float prototypes[N_PROT][3] = {
  {0.265, 0.380, 0.356}, // Black
  {0.278, 0.376, 0.346}, // White
  {0.246, 0.393, 0.361}, // Green
  {0.302, 0.361, 0.337}, // Red
  {0.287, 0.378, 0.336}, // Yellow
  {0.238, 0.391, 0.371}  // DarkSkyBlue
};
const char* labels[N_PROT] = {"Black","White","Green","Red","Yellow","DarkSkyBlue"};

// max allowed distance to accept a match (tune if needed)
float DIST_THRESHOLD = 0.06; // start forgiving; can lower if mislabels

void setup() {
  Serial.begin(9600);
  delay(200);
  Serial.println("Chromeye NN classifier (3 cycles -> majority -> 15s pause)");
  pinMode(LDR_PIN, INPUT_PULLUP);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  analogWrite(RED_PIN,0);
  analogWrite(GREEN_PIN,0);
  analogWrite(BLUE_PIN,0);
}

// helper to set LEDs
void setLEDs(int r, int g, int b) {
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
  delay(30);
}

// read averaged LDR while LED is ON
int readLDRavg() {
  long s = 0;
  for (int i=0;i<SAMPLES;i++){
    s += analogRead(LDR_PIN);
    delay(4);
  }
  return (int)(s / SAMPLES);
}

// convert raw ADC triplet -> normalized reflectance Rn,Gn,Bn
void rawToNorm(float Rraw, float Graw, float Braw, float &Rn, float &Gn, float &Bn) {
  float rR = 1023.0f - Rraw;
  float rG = 1023.0f - Graw;
  float rB = 1023.0f - Braw;
  float sum = rR + rG + rB;
  if (sum < 1e-6) sum = 1.0f;
  Rn = rR / sum;
  Gn = rG / sum;
  Bn = rB / sum;
}

// nearest prototype; returns index and distance via reference
int nearestProto(float Rn, float Gn, float Bn, float &outDist) {
  float bestD = 1e6;
  int bestI = -1;
  for (int i=0;i<N_PROT;i++){
    float dr = Rn - prototypes[i][0];
    float dg = Gn - prototypes[i][1];
    float db = Bn - prototypes[i][2];
    float d = sqrt(dr*dr + dg*dg + db*db);
    if (d < bestD){ bestD = d; bestI = i; }
  }
  outDist = bestD;
  return bestI;
}

// measure one cycle (R,G,B) and classify it; returns prototype index or -1 if unknown
int measureAndClassifyCycle(int cycleNum) {
  // RED on
  setLEDs(LED_BRIGHT,0,0);
  delay(LED_ON_MS);
  int Rraw = readLDRavg();
  setLEDs(0,0,0);
  delay(60);

  // GREEN on
  setLEDs(0,LED_BRIGHT,0);
  delay(LED_ON_MS);
  int Graw = readLDRavg();
  setLEDs(0,0,0);
  delay(60);

  // BLUE on
  setLEDs(0,0,LED_BRIGHT);
  delay(LED_ON_MS);
  int Braw = readLDRavg();
  setLEDs(0,0,0);
  delay(60);

  // print raw
  Serial.print("Cycle "); Serial.print(cycleNum);
  Serial.print(" | Raw R="); Serial.print(Rraw);
  Serial.print(" G="); Serial.print(Graw);
  Serial.print(" B="); Serial.print(Braw);

  // normalize
  float Rn, Gn, Bn;
  rawToNorm(Rraw, Graw, Braw, Rn, Gn, Bn);
  Serial.print(" | Norm R="); Serial.print(Rn,3);
  Serial.print(" G="); Serial.print(Gn,3);
  Serial.print(" B="); Serial.print(Bn,3);

  // nearest prototype
  float dist;
  int idx = nearestProto(Rn, Gn, Bn, dist);

  if (idx >= 0 && dist <= DIST_THRESHOLD) {
    Serial.print(" -> "); Serial.print(labels[idx]);
    Serial.print(" (d="); Serial.print(dist,3); Serial.println(")");
    return idx;
  } else {
    Serial.print(" -> UNKNOWN");
    Serial.print(" (bestd="); Serial.print(dist,3); Serial.println(")");
    return -1;
  }
}

void loop() {
  Serial.println("=== NEW BATCH ===");
  int votes[N_PROT]; for (int i=0;i<N_PROT;i++) votes[i]=0;
  int unknowns = 0;

  for (int c=1;c<=CYCLES;c++){
    int idx = measureAndClassifyCycle(c);
    if (idx >= 0) votes[idx]++; else unknowns++;
    delay(300);
  }

  // majority vote
  int bestI = -1; int bestV = 0;
  for (int i=0;i<N_PROT;i++){
    if (votes[i] > bestV) { bestV = votes[i]; bestI = i; }
  }

  Serial.print("Batch result: ");
  if (bestV >= 2) {
    Serial.print(labels[bestI]);
    Serial.print(" (votes="); Serial.print(bestV); Serial.println(")");
  } else {
    // if no prototype got >=2 votes, report unknown or the single top
    if (bestV == 1) {
      Serial.print(labels[bestI]);
      Serial.print(" (weak, votes=1) - maybe ");
      Serial.print(labels[bestI]); Serial.println("");
    } else {
      Serial.println("UNKNOWN");
    }
  }

  Serial.print("Pausing ");
  Serial.print(PAUSE_MS/1000);
  Serial.println("s before next batch\n");
  delay(PAUSE_MS);
}
