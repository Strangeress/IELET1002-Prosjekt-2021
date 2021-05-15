#Importerer moduler
import requests
from datetime import date,datetime
import pandas as pd
from os import path
import json
import Solar_production as sp
import snittpris_strm as ss
import bookingregister_v2 as br

def givePath(ID):                                                                   # En funskjon for å gi path til filene vi jobber med
    if ID == 1:
        return("csv/CurrentWeekKitchen.csv")
    if ID == 2:
        return("csv/CurrentWeekBathroom.csv")
    if ID == 3:
        return("csv/CurrentWeekLivingroom.csv")
    if ID == 4:
        return("csv/PowerHistory.csv")

def getPowerPrices():
    today = date.today()                                                            # Henter dagens dato
    
    url = 'https://norway-power.ffail.win/?zone=NO3&date={}'.format(today)          # URL for etterspørring av dagens strømpriser
    
    res = requests.get(url)                                                         # Vi etterspør dagens strømpriser
    
    data = res.json()                                                               # Henter ut prisene i json-format
    
    prices_df = pd.DataFrame.from_dict(data).transpose().drop(columns=['valid_from','valid_to'])    # Dropper unødvendige kolonner
    prices_df.reset_index(level=0,inplace=True)                                                     # Resetter index for at den blir lettere å jobbe med
    prices_df.columns=['unix','NOK']                                                                # Navngir kolonnene i tråd med hvordan vi jobber med dataene i andre dataframes
    
    for i in range(24):                                                                                                             # For hver linje i dataframen
        prices_df.at[i,'unix'] = int(datetime.timestamp(pd.to_datetime(prices_df.iloc[i,0]).tz_localize('Europe/Oslo')))+2*3600     # Vi gjør om verdiene i unix-kolonnen til unix tid

    return(prices_df)                                                               # Returnerer dataframen med unix tid og priser per time

def calculatePower(room):                                                           # Funksjon for å kalkulere strømforbruk per fellesareal
    dataFrame = pd.read_csv(givePath(room))                                         # Vi henter CSV-fila for rommet vi skal lese av
    dataFrame = dataFrame.astype({"unix": str})                                     # Behandler den slik at kolonnene er slik vi vil ha de
    dataFrame = dataFrame.set_index('unix')                                         # Behandler den slik at kolonnene er slik vi vil ha de
    
    powerConsumed = 0                                                               # Variabel for hvor mye strøm rommet har forbrukt
    powerPrice = 0                                                                  # Variabel for hvor mye strømmen vil koste
    hasCalculatedGenericPower = False                                               # Boolsk variabel for å passe på at vi ikke kjører feil del av funksjonen flere ganger
    
    prices_df = getPowerPrices()                                                    # Her importeres dagens strømpriser i en dataframe med index 0-23, unix tid per time, og strømpris for gitte time
    
    for i in range(1,7):                                                            # Vi vil gå igjennom booking-dataframen fra topp til bunn, en gang per rom (1-6)
        pricePos = 0                                                                # Variabel for posisjonen i pris-dataframen vi befinner oss i
        j = prices_df.iloc[0,0]                                                     # Variabel lik første unix-tid i prisdataframen. Dette vil alltid være midnatt. 
        while j < prices_df.iloc[23,0]+3600:                                        # Her sier vi at vi vil lete gjennom alle unix-verdier fra dagens midnatt til neste midnatt
            if prices_df.iloc[pricePos,0] == j:                                     # Hvis vi finner unix-tiden i pris-dataframen vet vi at dette er prisen for den neste timen, og det er denne prisen vi må regne med
                currentPrice = prices_df.iloc[pricePos,1]                           # Vi setter en variabel lik den prisen vi skal regne med
                pricePos += 1                                                       # Inkrementerer pris-posisjon for å være klar for å snappe opp neste times pris
                if pricePos > 23:                                                   # En sjekk for å passe på at programmet ikke krasjer ved å lete etter en index som ikke finnes
                    pricePos = 23
                if hasCalculatedGenericPower == False:                              # Dette er en funksjon vi bare vil kjøre en gang. For ytterligere forklaring se funksjon genericPower()
                    powerConsumed += genericPower(room,pricePos)                    # Legger til rommets strømforbruk i den løpende totalen
                    powerPrice += genericPower(room,pricePos) / 1000 * currentPrice # Legger til hva det vil koste i den løpende totalen
                    

            timeBooked = 0                                                          # Setter en variabel for å holde øye med hvor mange 5-minutters bolker vedkommende har booket på rad
            if dataFrame.loc[str(j),str(i)] > 0:                                    # Vi leter igjennom booking-dataframen og ser om vi finner starten på en booking
                for k in range(j,j+14400,300):                                      # Hvis vi finner en booking så er vi forberedt på å lete opptil 4 timer frem i tid etter slutten på bookingen
                    if dataFrame.loc[str(k),str(i)] > 0:                            # Sjekker om vedkommende har en booking for denne tiden
                        timeBooked += 1                                             # Hvis ja, så legger vi til 1 til denne variabelen
                        j += 300                                                    # Og inkrementerer j med 5 minutter for å ikke sjekke denne tiden igjen videre i programmet
                    else:                                                           # Hvis vi ikke finner en videre booking
                        break                                                       # Da hopper vi ut av for-loopen
                        
                # Under kaller vi på funksjonen bookingPower, med fellesrom-ID, hvor mange 5-minutter som er booket, og antall personer booket
                powerConsumed += bookingPower(room,timeBooked,dataFrame.loc[str(j-300),str(i)]) # Legger til i foreløpig totalt strømforbruk
                powerPrice += bookingPower(room,timeBooked,dataFrame.loc[str(j-300),str(i)]) / 1000 * currentPrice # Legger til i foreløpig total strømpris
                
            j += 300                                                                # Inkrementerer unix-tid til neste 5-minutt
        hasCalculatedGenericPower = True                                            # Nå har vi kjørt igjennom funksjonen 24 ganger, en gang per time, så vi trenger ikke regne mer på fellesarealene
    return(powerConsumed,powerPrice)                                                # Vi returnerer her det totale strømforbruket for fellesarealet. Både for tiden som er booket per rom, og for det faste strømforbruket

def genericPower(room,ToD):                                                         # RomID og Time of Day
    if room == 1:                                                                   # Kjøkken bruker strøm på kjøleskap
        return(440/24)                                                              # Strømforbruket på kjøkkenet endres ikke avhengig av tid
    if room == 2:                                                                   # Baderom bruker strøm på varmekabler
        if ToD < 7:                                                                 # Hvis klokken er mellom midnatt og 6 på morgenen
            return(25*10)                                                           # Varmekabler på halv effekt ganger antall kvadratmeter
        else:
            return(50*10)                                                           # Varmekabler på full effekt ganger antall kvadratmeter
    if room == 3:                                                                   # Stuen har en panelovn som justeres etter tiden på dagen
        if ToD < 7:                                                                 # Hvis klokken er mellom midnatt og 6 på morgenen
            return(250)                                                             # Panelovn på halv effekt
        else:
            return(500)                                                             # Panelovn på full effekt

def bedroomPower(room):                                                             # Funksjon for å regne ut hvor mye strøm hvert rom vil forbruke avhengig av rominnstillingene de har satt for øyeblikket
    TOKEN = "eyJhbGciOiJIUzI1NiJ9.eyJqdGkiOiI1MjI3In0.qmxoTz81LqxwwtambhnmU8PdpDy7LImXuhfdBYw2vq0" # Token for CoT
    
    data_1={'Key':findRoomKey(room),'Token':TOKEN}                                  # Legger token og key inn i variablen data_1. For info om findRoomKey, se funksjonen
    response=requests.get('https://circusofthings.com/ReadValue',params=data_1)     # Etterspør registeret fra CoT   

    datahandling=json.loads(response.content)                                       # Legger json-svaret i en variabel
    value = str(int(datahandling["Value"]))                                         # Vi ønsker registeret som string
    status = value[-1:]                                                             # Status er det første tallet i registeret som viser om vedkommende er hjemme
    if status == 2:                                                                 # Hvis status tilsier at vedkommende er på ferie
        return(0)                                                                   # Alle strømforbrukende apparater på rommet er slått av. Strømforbruket er null
    else:                                                                           # Hvis vedkommende er hjemme eller i nærområdet
        consumption = int(value[1:3])/30*600*24                                     # Vi ser for oss 600w ovner som har forbruk proposjonalt med temperatur (0-30). Innstilt temperatur på termostat henter vi fra tall 2 og 3 i registeret
        return(consumption, consumption / 1000 * ss.avg_price_calc())               # Vi returnerer dagsforbruk og dette forbruket ganget med gjennomsnittsprisen for strøm denne dagen

def findRoomKey (bedRoom):                                                          # I denne funksjonen returnerer vi CoT Key for rommet vi vil regne strømforbruk på
    if bedRoom == 1:                                                                # For demonstrasjon og for å spare CoT bruker vi ett signal for å representere alle rommene
        return(7284)
    elif bedRoom == 2:                                                              # Ønsker vi å legge til flere rom er det bare å endre på verdiene her
        return(7284)
    elif bedRoom == 3:
        return(7284)
    elif bedRoom == 4:
        return(7284)
    elif bedRoom == 5:
        return(7284)
    elif bedRoom == 6:
        return(7284)

def bookingPower(room,duration,people):                                             # Her regner vi strømmen forbrukt på en gitt booking
    if room == 1:                                                                   # Kjøkken bruker strøm på stekeovn
        if duration > 2:                                                            # Hvis kjøkkenet er booket i et kvarter eller mer antar vi bruk av stekeovn
            return(duration * 1000/12 * people)                                     # Lengde på bruk, halvparten av en 2000W ovn, antall personer som bruker den
        else:
            return(0)
    if room == 2:                                                                   # Baderom bruker strøm på dusjing
        if duration > 2:                                                            # Hvis badet er booket i et kvarter eller mer antar vi dusj
            return(duration * 2500)                                                 # Strømforbruk av 5 minutter dusjing
        else:
            return(0)
    if room == 3:                                                                   # Stuen har en TV
        return(duration * 100/12)                                                   # TVen forbruker 100W per time, uavhengig av antall personer

def checkCurrentWeekPresent ():                                                     # En funksjon for å forsikre oss om at CSV-fila for strømhistorikk eksisterer
    
    prices_df = getPowerPrices()                                                    # Henter dagens strømpriser, med dagens midnatt lett tilgjengelig
        
    dataFrame = pd.DataFrame(columns=["unix", "price", "consumption","production"]) # Lager ny dataframe med kolonnene vi ønsker
    
    dataFrame["unix"] = prices_df.iloc[0,0]                                         # Setter første verdien i unix-kolonnen lik dagens midnatt
    
    dataFrame= dataFrame.set_index('unix')                                          # Setter unix-kolonnen til index
    
    if path.exists(givePath(4)) == False:                                           # Hvis filen ikke eksisterer
        
        dataFrame.to_csv(givePath(4))                                               # Da lagres dataframen som ny fil
    
    return()

def saveToCSV(powerConsumed,powerPrice,powerProduced):                              # En funksjon for å lagre alt som har med strøm å gjøre i en historisk CSV-fil
    
    dataFrame = pd.read_csv(givePath(4))                                            # Vi leser av filen som allerede eksisterer
    dataFrame = dataFrame.astype({"unix": str})                                     # Vi behandler kolonnenene slik at vi kan jobbe enkelt med dataframen
    dataFrame = dataFrame.set_index('unix')                                         # Vi behandler kolonnenene slik at vi kan jobbe enkelt med dataframen
    
    prices_df = getPowerPrices()                                                    # Henter dagens strømpriser, med dagens midnatt lett tilgjengelig
    
    dataFrame.at[str(prices_df.iloc[0,0]), "price"] = powerPrice                    # Setter at for index lik midnatt i dag, skal kolonnen price inneholde dagens utregnede strømpris
    dataFrame.at[str(prices_df.iloc[0,0]), "consumption"] = powerConsumed           # Setter at for index lik midnatt i dag, skal kolonnen consumption inneholde dagens utregnede strømforbruk
    if powerProduced > 0:                                                           # Hvis vi har oppgitt produsert strøm (som vi gjør kun en gang per dag, rundt midnatt)
        dataFrame.at[str(prices_df.iloc[0,0]), "production"] = powerProduced        # Setter at for index lik midnatt i dag, skal kolonnen production inneholde dagens utregnede strømproduksjon
    
    dataFrame.to_csv(givePath(4))                                                   # Vi lagrer dataene tilbake i samme filen
    
    uploadToCoT(powerConsumed,powerPrice)                                           # Vi laster opp verdiene i CoT

def uploadToCoT(powerConsumed,powerPrice):                                          # Funksjon for å laste opp strømforbruk og pris i CoT
    
    TOKEN = "eyJhbGciOiJIUzI1NiJ9.eyJqdGkiOiI1MjI3In0.qmxoTz81LqxwwtambhnmU8PdpDy7LImXuhfdBYw2vq0" # CoT token
    
    data_1={'Key':24913,'Value':powerConsumed,'Token':TOKEN}                        # Strømforbruk lagres i data_1

    requests.get('https://circusofthings.com/WriteValue', params=data_1)            # Vi laster opp strømforbruk til CoT
    
    data_1={'Key':23091,'Value':weeklyPrice(),'Token':TOKEN}                        # Den totale strømprisen for uka lagres i data_1. Hentes med funksjon                             

    requests.get('https://circusofthings.com/WriteValue', params=data_1)            # Vi laster opp Strømpris til CoT
    
def weeklyPrice():                                                                  # Funksjon for å regne ut foreløpig strømregning denne uka
    
    prevMonday = br.getMonday()                                                     # Vi bruker en funksjon som finner unix-tiden for siste mandag, ved midnatt

    dataFrame = pd.read_csv(givePath(4))                                            # Vi åpner historiske strømdata
    dataFrame = dataFrame[dataFrame["unix"] >= prevMonday]                          # Isolerer radene med tid lik eller etter mandag ved midnatt
    dataFrame = dataFrame.astype({"unix": str})                                     # Vi behandler kolonnenene slik at vi kan jobbe enkelt med dataframen
    dataFrame = dataFrame.set_index('unix')                                         # Vi behandler kolonnenene slik at vi kan jobbe enkelt med dataframen
    
    sumdf_value = dataFrame.sum(axis=0,skipna=True).iloc[0:1]                       # Vi summerer alle dagenes strømpris fra og med siste mandag o
    
    weekPrice = sumdf_value[0]                                                      # Henter ut summen av strømprisene
    
    return(weekPrice)                                                               # Returnerer den totale strømprisen for denne uka


def mainPower(isMidnight):                                                          # Hovedfunksjonen for denne modulen

    checkCurrentWeekPresent()                                                       # Vi sjekker om filen for historiske strømdata eksisterer
    
    totalConsumption = 0                                                            # Variabel for dagens totale strømforbruk
    totalPrice = 0                                                                  # Variabel for dagens totale strømregning
    for i in range(1,4):                                                            # Løkke som kjøres en gang per fellesareal
        consumption, price = calculatePower(i)                                      # Vi finner forbruket for dette rommet
        totalConsumption += consumption                                             # Legger til dagsforbruket for rommet i det totale dagsforbruket
        totalPrice += price                                                         # Legger til dagsprisen for rommet i den totale dagsprisen
    
    
    for i in range(1,7):                                                            # Løkke som kjøres en gang per soverom
        consumption, price = bedroomPower(i)                                        # Vi finner forbruket for dette soverommetrommet
        totalConsumption += consumption                                             # Legger til dagsforbruket for rommet i det totale dagsforbruket
        totalPrice += price                                                         # Legger til dagsprisen for rommet i den totale dagsprisen
    
    if isMidnight == True:                                                          # Hvis vi har spesifisert at det er midnatt
        saveToCSV(totalConsumption,totalPrice,sp.solarMain())                       # Etterspør funksjonen med antatt strømproduksjon
    else: 
        saveToCSV(totalConsumption,totalPrice,0)                                    # Etterspør funksjonen uten antatt strømproduksjon
    
    return(totalConsumption,totalPrice)                                             # Returnerer dagens forbruk og pris

