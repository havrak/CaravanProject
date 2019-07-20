---
geometry: margin=2cm
fontsize: 12pt
---
# Přípravy

|položka|stauts|
|-----|-----|
|ESP NOW duplex + route table| Funguje |
|NTP pomocí ethernetu na OLIMEX| Funguje |
|EEPROM | Funguje|
|Rozložení obrazovky | Předběžně vyřešeno |
|Komunikace NEXTION \ OLIMEX| Nefunguje |

+ EEPROM - návody na internetu jsou chybné
+ NTP - potřeba GPS na časová pásma + uživatel nastaví letní/zimní čas (nevypadá že je automaticky)

# Rozložení obrazovy
Odzkoušet co vypadá dobře, špatně se modeluje v malování.
Předběžně, dole (možná nahoře) bude status bar, chlívky děleny čárou, ta není kolmá na stranu.
Po kliknutí se změní vršek na danou stránku.
Na ní v levém horním rohu bude tlačítko zpět.
Prvky ovládání (jak vybrat teplotu podlahy atd.) zvolím jak co bude fungovat.

Jinak budou zobrazována data, na kterých jsem se již domluvily, jen nevím zda je potřeba GPS, zda by nestačila jen ikonka v pravém horním rohu v případě že je málo družic, protože potom stejně nepude hledat počasí pro aktuální polohu.

\newpage


# Voda
Celkově by neměla zabrat déle jak 1 až 2 dny.

### Cíle části
|položka|status|
|-----|-----|
|Je připojená přípojka + automatické dopouštění| Nefunguje |
|Průtokoměr - odečet vody | Nefunguje |
|Teplota vody | Nefunguje - stará se o to centrála |

### Možné přidat
|položka|status|
|----|----|
|Statistiky o spotřebě | Nefunguje|

\newpage

# Elektřina
Základ by měl být relativně rychlý - mělo by stačit jen přečíst data, .
Očekávám den,

### Cíle části
|položka|status|
|-----|-----|
|Stav baterie| Nefunguje |
|Odběr| Nefunguje |

### Možné přidat
|položka|status|
|----|----|
|Statistiky o spotřebě | Nefunguje|
|Úsporný mód| Nefunguje|

\newpage

# Poziční systém + zabezpečení
Část by se měla udělat, stačí jen poslat GPS pro počasí.
Nevím co dál by měla dělat, pokud by měla poslat SMS v případě pohybu, tak by bylo potřeba použít sim kartu na LTE, což nevím zda lze. Předpokládám pár dní.

Potřeba získat data ze serveru - [Repozitář o CURL na ESP32](https://github.com/loboris/ESP32_curl_exampley).

### Cíle části
|položka|stauts|
|-----|-----|
|Získat GPS koordináty| Nefunguje |
|Akcelerometr| Nefunguje |
|počasí přes wttr.in| Nefunguje|

\newpage

# Topení
Důležitá oboustranná komunikace (ovládáno výhradně uživatelem).

### Cíle části
|položka|status|
|-----|-----|
|Zapínaní jednotlivých okruhů| Nefunguje |
|Teplotní čidla | Nefunguje |

### Možné přidat
|položka|status|
|----|----|
|Úsporný mód| Nefunguje|
|Vypnout topení pokud jsou otevřená okna (Velké rozdíly mezi spodem a vrchem nebo něco podobného) | Nefunguje|

\newpage

# Teploty
Stačí jen přidat odeslání na centrálu, potřebuji aktuální program.

### Cíle části
|položka|status|
|-----|-----|
|Získání teplot na jednotlivých místech| Nefunguje (Bude jen stačit přidat odeslání na centrálu) |
|Zobrazení na obrázku| Obrázek zeditován|

\newpage

# LTE / domácí AP
Mikrotik má doma i máma takže na ten telnet se podívám, pak by to mělo být relativně rychlé.

### Cíle části
|položka|status|
|-----|-----|
|Možnost přepnout zdroj internetu| Nefunguje|

### Možné přidat
|položka|status|
|----|----|
|Přepínat automaticky dle GPS pozice| Nefunguje|
