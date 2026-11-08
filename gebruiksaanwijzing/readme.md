# Gebruiksaanwijzing

## opladen batterijen

### Batterijen opladen
1. Zet de aan/uit-schakelaar uit.
2. Trek de connectoren van de batterijen uit de connectoren op het protoboard.
3. Sluit de batterijen aan op de lader.
4. Steek een USB C kabel in de lader en in het stopcontact.
5. Laad de batterijen op tot de lader aangeeft dat ze vol zijn.
6. Koppel de lader los en steek de connectoren van de batterijen terug in de connectoren op het protoboard.
7. Zet de aan/uit-schakelaar aan.


## draadloze communicatie
### verbinding maken
1. Zet de aan/uit-schakelaar aan.
2. Open de WiFi-instellingen van je smartphone of laptop.
3. Verbind met het netwerk **Linefollower**.
4. Voer het wachtwoord **linefollower** in.
5. Open in de browser **http://192.168.4.1**.

Meldt je smartphone dat het netwerk geen internet heeft? Kies dan **Verbonden blijven**.

## commando's
Alle commando's geef je via de webpagina.
Na het aanpassen van de instellingen klik je op **Toepassen en opslaan**. De waarden blijven bewaard als je de robot uitzet.

## kalibratie
2. Houd alle sensoren boven het zwarte vlak.
3. Klik op **Kalibreer ZWART**. Wacht op de melding *Zwart gekalibreerd*.
4. Houd alle sensoren boven de witte ondergrond.
5. Klik op **Kalibreer WIT**. Wacht op de melding *Wit gekalibreerd*.
6. Verschijnt de melding *Klaar om te starten*? Dan is de kalibratie gelukt.
7. Verschijnt de melding *Let op: bij sensor A… is zwart niet hoger dan wit*? Kalibreer dan opnieuw.
8. Kalibreer opnieuw bij ander licht of een andere ondergrond.

De robot kan pas starten als zowel zwart als wit gekalibreerd zijn. De kalibratie blijft bewaard als je de robot uitzet.

## settings
De robot rijdt stabiel met volgende parameters:  

| Parameter | Waarde |
|---|---|
| Cyclustijd | [waarde] µs |
| Basissnelheid (power) | [waarde] |
| diff | [waarde] |
| Kp | [waarde] |
| Ki | [waarde] |
| Kd | [waarde] |

## start/stop button
De drukknop zit rechts vooraan op de robot. 

- **1e druk:** de robot start na de startvertraging. De start-led gaat aan.
- **2e druk:** de robot stopt onmiddellijk. De start-led gaat uit.
- De robot start alleen als zwart en wit gekalibreerd zijn.
- De knop werkt ook terwijl je met de webpagina verbonden bent.
