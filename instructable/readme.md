# Instructable

Een instructable is een stappenplan - zonder verdere uitleg - hoe je vertrekkend van de bill of materials en gebruik makend van de technische tekeningen de robot kan nabouwen. Ook de nodige stappen om de microcontroller te compileren en te uploaden staan beschreven.

### stap 1
bestel alle componenten uit de bill of materials

### stap 2
verzamel het gereedschap
- Soldeerbout en tin
- Lijmpistool (hot glue)
- Striptang en kniptang
- Schroevendraaiers
- Multimeter
- Computer met internetverbinding
- Smartphone of laptop met WiFi

### stap 3
Bevestig de motoren op het protoboard met behulp van de motorhouders.

### stap 4
Bevestig het zwenkwiel op het protoboard.

### stap 5
Plaats de pinheaders (female) voor de motordriver, de Arduino Nano ESP32 en de sensoren op het protoboard en soldeer ze vast aan het protoboard.

### stap 6
Plaats de twee leds, de drukknop, de aan/uit-schakelaar en de batterijconnectoren op het protoboard en soldeer ze vast aan het protoboard.

### stap 7
Soldeer de pinheaders (male) vast aan de motordriver.

### stap 8
Soldeer de pinheaders (male) vast aan de sensoren.

### stap 9
Soldeer de verbindingen tussen de ESP32 en de sensoren.

### stap 10
Soldeer de verbindingen tussen de ESP32 en de motordriver.

### stap 11
Soldeer de verbindingen tussen de motoren en de motordriver.

### stap 12
Soldeer de verbindingen tussen de ESP32 en de drukknop.
- Verbind de ene pin van de drukknop met D2.
- Verbind de andere pin van de drukknop met GND.

### stap 13
Soldeer de verbinding tussen de led die aantoont dat het programma gestart is en de ESP32.
- Verbind D12 met de anode (lange pin) van de led.

### stap 14
Soldeer de weerstand tussen de led die aantoont dat het programma gestart is en de GND.
- Verbind de kathode (korte pin) van de led via de weerstand met GND.

### stap 15
Soldeer de verbindingen tussen de batterijconnectoren om de batterijen in serie te plaatsen.

### stap 16
Neem het 2e protoboard en zaag er een deel van zodat je dit deel kan monteren boven het zwenkwiel.

### stap 17
Lijm de spanningsregelaar vast op dit protoboardje met behulp van hotglue.

### stap 18
Lijm het protoboardje met de spanningsregelaar boven het zwenkwiel vast op het protoboard met hotglue.

### stap 19
Soldeer de verbinding tussen de batterijconnectoren en de primaire kant van de spanningsregelaar.

### stap 20
Stel de spanningsregelaar in op een uitgangsspanning van 6 Volt.
- Sluit de batterijen tijdelijk aan op de batterijconnectoren.
- Meet de uitgangsspanning met de multimeter.
- Draai aan de instelschroef tot de uitgangsspanning 6 Volt is.
- Koppel de batterijen weer los.

### stap 21
Soldeer de verbinding tussen de secundaire kant van de spanningsregelaar en de aan/uit-schakelaar.

### stap 22
Soldeer de secundaire kant van de aan/uit-schakelaar naar de motordriver (VM), de aan/uit-led en de ESP32 (VIN).

### stap 23
Soldeer de weerstand tussen de aan/uit-led en de GND.

### stap 24
Controleer de bedrading met de multimeter.
- Controleer dat er geen kortsluiting is tussen + en GND.
- Controleer dat alle GND-verbindingen met elkaar verbonden zijn.
- Controleer dat er geen kortsluiting is tussen aangrenzende soldeerpunten.

### stap 25
Monteer de wielen op de motoren.

### stap 26
Bevestig de batterijen op het protoboard met behulp van hotglue.

### stap 27
Klik de sensoren, ESP32 en de motordriver in de pinheaders.

### stap 28
Download en installeer de Arduino IDE via [arduino.cc/en/software](https://www.arduino.cc/en/software) indien je deze nog niet hebt.

### stap 29
Installeer de board-package.
- Open **Tools → Board → Boards Manager**.
- Zoek **Arduino ESP32 Boards** en klik op **Install**.

### stap 30
Open het programma.
- Download of clone deze repository.
- Open de finale code in de Arduino IDE.

### stap 31
Verbind de microcontroller met de computer met een USB-C-datakabel.

### stap 32
Stel de microcontroller in.
- Kies **Tools → Board → Arduino ESP32 Boards → Arduino Nano ESP32**.
- Kies **Tools → Pin Numbering → By Arduino pin (default)**.
- Kies **Tools → Port** en selecteer de poort van de Nano ESP32.

### stap 33
Compileer het programma.
- Klik op **Verify** (vinkje).
- Wacht tot de melding *Done compiling* verschijnt.

### stap 34
Upload het programma.
- Klik op **Upload** (pijl).
- Wacht tot de melding *Done uploading* verschijnt.
- Mislukt de upload? Dubbelklik snel op de resetknop van de ESP32, kies opnieuw de poort en upload opnieuw.

### stap 35
Als het programma geüpload is mag je de usb kabel uit de ESP32 halen.

### stap 36
Steek de connectoren van de batterijen in de connectoren op het protoboard.

### stap 37
Zet de aan / uit schakelaar aan.

### stap 38
Verbind met de robot.
- Open de WiFi-instellingen van je smartphone of laptop.
- Verbind met het netwerk **Linefollower**.
- Voer het wachtwoord **linefollower** in.
- Open in de browser **http://192.168.4.1**.

### stap 39
Test de webpagina.
- Klik op **START**: de LED gaat aan.
- Klik op **STOP**: de LED gaat uit.
- Stel een cyclustijd in en klik op **Toepassen en opslaan**.
- Controleer onder *Cyclustijd* dat de rekentijd en de werkelijke periode zichtbaar zijn.
- Zet de robot uit en weer aan: de ingestelde waarden zijn bewaard.

### stap 40
Controleer de draairichting van de motoren.
- Houd de robot in de lucht zodat de wielen vrij draaien.
- Laat de motoren kort vooruit draaien met het eindprogramma.
- Draait een motor achteruit? Verwissel de pinnen van die motor in het Arduino programma.

### stap 41
Zet de robot op het parcours en kalibreer de sensoren.
- Houd alle sensoren boven de zwarte lijn en klik op **Kalibreer ZWART**.
- Houd alle sensoren boven de witte ondergrond en klik op **Kalibreer WIT**.

### stap 42
De robot is klaar.
- Start de robot via de drukknop of via de webpagina.
