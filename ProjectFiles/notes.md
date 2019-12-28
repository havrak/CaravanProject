+ updateYourData will check length and if it doesn't correspond send false - that will be counted as invalid message -> that way we can identify hostile units (at least in some way)

+ while cycle to connect to mikrotik, it takes some tries

gps.satellites.value(.satellites.value()
gps.location.lat()
gps.location.lng()

+ tlačítko
	+ po stisknutí se odhalí SSID
	+ po dalším stisknutí ho zase schová
+ jednotka
	+ neustále skenuje
	+ najde li jednotku s SSID CARAVAN_CENTRAL_UNIT tak se spáruje
	+ pokud spárování proběhne úspěšně tak pošle svůj kód (100+)
	+ poté bude čekat na odpověď 92 -- pokud dostane nastaví si centrálu a zamkne si ji
	+ zapíše si čas kdy přišla 92
	+ nepřijde li nic po dobu 2 pak se zkusí znovu spárovat

+ centrála
	+ nechá se spárovat
	+ přijde jí zpráva s číslem, pokud je v range, tak si vytvoří esp_info, přidá ho do pole jednotek, spáruje se nazpátek, pošle kód 92, nastaví si datum poslední zprávy na millis()
	+ pravidelně posílá ping - číslo 88



/interface ethernet poe set ether4 poe-out=off
/interface ethernet poe set ether4 poe-out=forced-on


