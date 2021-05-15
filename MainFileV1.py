import sys
sys.path.append('/home/pi/prosjekt/modules')                                         # Her definerer vi hvor vi vil hente våre egne moduler fra

import bookingregister_v2 as br
import powerMath2 as pm
import hentvaerdata as hv
import snittpris_strm as ss

import os, time

import schedule


os.environ['TZ'] = 'Europe/Oslo'
time.tzset()                                                                        # Setter tidssone for at utregningene våre skal være korrekte da vi opererer med unix-tid

br.checkCurrentWeekPresent()                                                        # Sjekker at booking-CSV-filene våre er tilstede, slik at vi kan utføre bookinger

prevMonday = br.getMonday()                                                         # Setter en variabel lik midnatt for mandag denne uken. Brukes for å holde styr på om vi er inne i en ny uke

def threeSecondJobs():                                                              # Alt her inne er tidssensitivt og bør ha så lite forsinkelser som mulig, og kjøres derfor hvert tredje sekund
    global prevMonday                                                               # Passer på at variabelen taes med inn i funksjonen
    for i in range(1,2):                                                            # Kjører gjennom en gang for hvert soverom, skal egentlig være range(1:7), men da vi kun har bygget ut funksjonalitet for ett rom i CoT kjører vi bare for rom 1
        signal = int(br.bookingRegisterCoT(br.findRoomKey(i)))                      # Setter en variabel lik det som står i CoT for romnummer i, som etterspørres med key som vi henter fra en annen funksjon
        if len(str(signal)) == 15:                                                  # Hvis signalets lengde tilsier at det er blitt gjort en booking går vi videre
            print(br.mainBooker(signal,i))                                          # Da kaller vi på hovedfunksjonen i bookingregister-modulen vi har laget
        else:                                                                       # Hvis det ikke blir oppdaget en booking
            print("No booking detected for room number " + str(i))                  # Gir feedback om at vi har sjekket men ikke funnet booking
    
    if br.checkForWeekChange(prevMonday) == "New week":                             # Sjekker om det er blitt ny uke
        prevMonday = br.getMonday()                                                 # Hvis ja, så setter vi variabelen lik denne nye ukens mandag ved midnatt

def hourlyJobs():                                                                   # Disse funskjonene kalles en gang i timen
    print(pm.mainPower(False))                                                      # Kaller på hovedfunksjonen i modulen powerMath2, med den boolske variabelen som definerer om det er midnatt eller ikke lik False
    hv.temp_upload()                                                                # Kaller på en funksjon i hentvaerdata som laster opp nåværende temperatur i smartmeteret
    
    
def dailyJobsBeforeMidnight():                                                      # Disse jobbene må kjøres ved midnatt, men før vi går inn i neste dag da det er data vi skal hente som er for dagen som har vært
    pm.mainPower(True)                                                              # Kaller på hovedfunksjonen i modulen powerMath2, med den boolske variabelen som definerer om det er midnatt eller ikke lik True

def dailyJobsAfterMidnight():                                                       # Disse jobbene må kjøres ved midnatt, men etter vi har gått inn i neste dag da det er data vi skal hente som er for dagen kommer
    ss.avg_price_upload()                                                           # Kaller på en funksjon som laster opp gjennomsnittsprisen for strøm de neste 24 timer
    
# Under bruker vi schedule for å kjøre funksjonene våre til tidene vi ønsker at de skal kjøre
schedule.every(3).seconds.do(threeSecondJobs) 
schedule.every().hour.at(":00").do(hourlyJobs)
schedule.every().day.at('23:57').do(dailyJobsBeforeMidnight)
schedule.every().day.at('00:15').do(dailyJobsAfterMidnight)

while True:                                                                         # Vi ønsker ikke at programmet skal stoppe, så vi har en evig loop
    schedule.run_pending()                                                          # Kommando for å få schedule til å fungere
