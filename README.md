# Esphome-Samsung-Climate

Questo progetto serve a controllare un climatizzatore Samsung sprovvisto di interfaccia wifi.
Nel mio caso ho effettuato la modifica su un Samsung Maldives.

![Card](https://github.com/mansellrace/Esphome-Samsung-Climate/blob/main/img/card.png) 


## Hardware

Il progetto utilizza un wemos D1 mini per il collegamento ad Home assistant, il circuito viene inserito all'interno del climatizzatore in prossimità della scheda frontale su cui è montato il ricevitore IR del climatizzatore. 
La nuova scheda si collega alla scheda Samsung in soli tre punti, prelevando alimentazione e collegandosi all'uscita del ricevitore IR.
In questo modo viene intercettata la sequenza di dati captata dal telecomando per sincronizzare lo stato su home assistant, ed iniettando sullo stesso pin i comandi generati dal wemos si può comandare il climatizzatore.

![Schema elettrico](https://github.com/mansellrace/Esphome-Samsung-Climate/blob/main/img/schema_samsung_ir.png)

Poichè il wemos lavora a 3.3v e il ricevitore IR lavora a 5v ho interposto un level shifter.

Il pin D6 è impostato come ricezione dati, il pin D7 come trasmissione.

![Scheda wemos](https://github.com/mansellrace/Esphome-Samsung-Climate/blob/main/img/scheda_wemos.jpg)
![Scheda samsung fronte](https://github.com/mansellrace/Esphome-Samsung-Climate/blob/main/img/scheda_samsung_fronte.jpg)
![Scheda samsung retro](https://github.com/mansellrace/Esphome-Samsung-Climate/blob/main/img/scheda_samsung_retro.jpg)

## Software
L'integrazione gestisce correttamente la modalità Fast, Quiet, l'oscillazione, le 4 velocità standard, le 5 modalità opeative (auto / caldo / freddo / deumidificazione / solo ventilazione). Quando si imposta il climatizzatore da telecomando, l'entità su home assistant viene aggiornata correttamente.

![more_info](https://github.com/mansellrace/Esphome-Samsung-Climate/blob/main/img/more_info.png)

Visto che l'entità di tipo climate viene mostrata a schermo con temperatura attuale e temperatura target, la temperatura attuale della stanza viene passata al climate prelevandola da un sensore ambientale, per evitare di visualizzare 0° a display.

L'integrazione è basata sulla libreria [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266), utilizzando il codice di altri progetti simili trovati sul web, riadattandoli al mio caso specifico.

Il file [irsamsung.h](https://github.com/mansellrace/Esphome-Samsung-Climate/blob/main/irsamsung.h) va inserito all'interno della cartella di home assistant config/esphome/

Successivamente aggiungi il nuovo wemos a esphome, e una volta che viene creata la configurazione base inserisci all'interno del file di configurazione il contenuto del file [esphome.yaml](https://github.com/mansellrace/Esphome-Samsung-Climate/blob/main/esphome.yaml). 
Della tua configurazione iniziale mantieni solo la OTA password e la API KEY. 
Sostituisci il nome della board nel secondo rigo, il nome delle entità switch, climate, il nome del sensore di temperatura.
