# Importerer moduler
import requests
import json
import pandas as pd
from datetime import date, datetime, timedelta      
import time
from os import path

TOKEN = "eyJhbGciOiJIUzI1NiJ9.eyJqdGkiOiI1MjI3In0.qmxoTz81LqxwwtambhnmU8PdpDy7LImXuhfdBYw2vq0" #Token som brukes til autentisering med CoT

kitchenPath = "csv/CurrentWeekKitchen.csv"                                          #Definerer filbanen for denne ukens kjøkkenfil
bathroomPath =  "csv/CurrentWeekBathroom.csv"                                       #Definerer filbanen for denne ukens baderomsfil
livingroomPath = "csv/CurrentWeekLivingroom.csv"                                    #Definerer filbanen for denne ukens stuefil


def findRoomKey (bedRoom):                                                          #Funksjon for å returnere key for behandling av data knyttet til et spesifikt rom i CoT
    if bedRoom == 1:                                                                #En if-setning for hvert rom
        return(15741)                                                               #Her fyller man in key, slik at man slipper å gjøre det på flere plasser i koden
    elif bedRoom == 2:
        return()
    elif bedRoom == 3:
        return()
    elif bedRoom == 4:
        return()
    elif bedRoom == 5:
        return()
    elif bedRoom == 6:
        return()


def getTime(unit):                                                                  #En funksjon som returnerer nåtid på ønsket format
    realTime = 1                                                                    #Funksjonen var tiltenkt å returnere enten faktisk tid eller en tid vi definerte under
    fakeTime = 1618189500                                                           #Vi så for oss at dette ville gjøre det mulig for oss å simulere systemet fra ønsket tid og med aksellerert tidsflyt. Vi bruker det ikke allikevel
    
    if realTime == 1:                                                               #Sjekker om vi vil bruke ekte tid eller falsk tid
        if unit == "unix":                                                          #Hvis vi ønsker tiden på formatet unix
            return(int(time.time()))                                                #Returner nåværende unixtid
        elif unit == "date":                                                        #Hvis vi ønsker tiden som dato
            return(datetime.fromtimestamp(time.time()))                             #Returnerer nåværende dato
        
    elif realTime == 0:                                                             #Hvis vi vil bruke falsk tid
        if unit == "unix":                                                          #Hvis vi ønsker tiden på formatet unix
            return(int(fakeTime))                                                   #Returnerer den falske tiden
        elif unit == "date":                                                        #Hvis vi ønsker tiden som dato
            return(datetime.fromtimestamp(fakeTime))                                #Returnerer den datoen til den falske tiden


def getMonday():                                                                    #En funksjon som returnerer denne ukens mandag ved midnatt i unixtid
    monday = getTime("date").date() - timedelta(days=datetime.today().weekday())    #Vi finner dagens dato og trekker fra dagens tall, fra 0. Eks, torsdag er ukedag 3. Torsdag - 3 dager = mandag
    mondayMidnight = datetime(monday.year, monday.month, monday.day)                #Vi finner midnatt for denne dagen
    return(int(datetime.timestamp(mondayMidnight)))                                 #Returnerer denne dagens midnatt som unixtid


def bookingRegisterCoT (KEY):                                                       # En funksjon for å hente informasjon fra CoT                                                                  # Token comes from the account that hosts the signal

    data_1={'Key':KEY,'Token':TOKEN}                                                # Slår sammen verdiene i et dictionary kalt "data_1"

    response=requests.get('https://circusofthings.com/ReadValue',params=data_1)     # Etterspør data fra CoT

    datahandling=json.loads(response.content)                                       # Henter json-verdiene fra svaret vi fikk fra CoT
    value = datahandling["Value"]                                                   # Plukker ut kun value fra svaret

    return value                                                                    # Returnerer value


def registerResponseCoT (value,KEY):                                                # En funksjon for å laste opp en verdi i CoT                                                                     # Token comes from the account that hosts the signal

    data_1={'Key':KEY,'Value':value,'Token':TOKEN}                                  # Slår sammen verdiene i et dictionary kalt "data_1"

    response=requests.get('https://circusofthings.com/WriteValue', params=data_1)   # Sender data til CoT
				 
    if(response.status_code==200):                                                  # Sjekker om CoT responderer med suksesskode
        return("success")                                                           # Returnerer success hvis det var vellykket
    else:                                                                           # Hvis CoT returnerer en annen kode
        print("error %d" % (response.status_code))                                  # Printer feilkoden


def bookingRegisterImport (register):                                               # En funksjon som bryter ned signalet til verdier vi kan bruke senere
    
    roomID = int(register[:1])                                                      # Hvilket rom som ønskes å bookes. Kjøkken er 1, bad er 2 og stue er 3
    duration = int(register[1:4])*60                                                # Lengden på bookingen i sekunder. Vi mottar lengden i minutter, så derfor ganger vi med 60
    timeStart = int(register[4:14])-7200                                            # Starttid for bookingen i unixtid, minus to timer da ESP32 sender feil tid
    people = int(register[14:15])                                                   # Antall personer som ønskes å bookes
    
    timeStop = timeStart + duration                                                 # Vi finner sluttid
    
    betweenNumbers = list(map(str, range(timeStart, timeStop, 300)))                # Vi finner alle tidspunkt mellom starttid og sluttid
    
    return (roomID, betweenNumbers, people)                                         # Returnerer verdiene vi trenger for å gjøre bookinger


def bookingRegistration (roomID, bedRoom, betweenNumbers, people):                  # En funksjon for å sjekke om bookingen er lov, og i så fall legge den inn i riktig fil
    
    df, roomMax = openCurrentWeek(roomID)                                           # Åpner dataframen til rommet vi vil sjekke samt maksantall på rommet
    
    for i in range(len(betweenNumbers)):                                            # For hver verdi i listen betweennumbers
        sumDF = int(df.at[betweenNumbers[i],"sum"])                                 # Henter verdien fra sum-kolonnen for tidspunktet vi vil sjekke
        sumNew = sumDF + people                                                     # Legger sammen antallet booket med antallet ønsket booket
    
        if sumNew > roomMax:                                                        # Hvis den nye totalen er større enn maksantallet tillatt
            if registerResponseCoT(333,findRoomKey(bedRoom)) == "success":          # Forsøker å laste opp til CoT at bookingen feilet, og sjekker at vi får respons fra CoT
                return("Max is reached")                                            # Returnerer feilmeldingen
            else:                                                                   # Hvis opplastingen feiler
                return("Max is reached but unable to respond")                      # RReturnerer feilmeldingen

    for i in range(len(betweenNumbers)):                                            # For hver verdi i listen betweennumbers
        df.at[betweenNumbers[i], str(bedRoom)] = people                             # Legger inn antall personer booket for hver tid vi ønsker å booke
    
    saveCurrentWeek(df,roomID)                                                      # Lagrer dataframen som fil med funksjonen
    if registerResponseCoT(111,findRoomKey(bedRoom)) == "success":                  # Prøver å laste opp suksesskode til CoT
        return("Booking successful")                                                # Hvis vi får respons fra CoT returnerer vi suksessmelding
    else:                                                                           
        return("Booking made but unable to respond")                                # Hvis vi ikke får respons fra CoT returnerer vi feilmelding
    
    
def mainBooker (signal,bedRoom):                                                    # Hovedskriptet som kjører når vi skal registrere booking
    roomID, betweenNumbers, people = bookingRegisterImport(str(signal))             # Behandler signalet og henter ut variablene vi trenger
    return(bookingRegistration(roomID, bedRoom, betweenNumbers, people))            # Prøver å legge inn bookingen og returnerer svaret fra funksjonen
    
    








# CSV EDIT FUNCTIONS

def saveCurrentWeek (dataFrame,roomID):                                             # Funksjon for å lagre dataframene som filer
    if roomID == 1:                                                                 # Sjekker hvilket rom det er snakk om
        dataFrame["sum"] = dataFrame[["1","2","3","4","5","6"]].sum(axis=1)         # Passer på at sumkollonnen er korrekt
        dataFrame.to_csv(kitchenPath)                                               # Lagrer filen på riktig bane
    if roomID == 2:                                                                 # Sjekker hvilket rom det er snakk om
        dataFrame["sum"] = dataFrame[["1","2","3","4","5","6"]].sum(axis=1)         # Passer på at sumkollonnen er korrekt
        dataFrame.to_csv(bathroomPath)                                              # Lagrer filen på riktig bane
    if roomID == 3:                                                                 # Sjekker hvilket rom det er snakk om
        dataFrame["sum"] = dataFrame[["1","2","3","4","5","6"]].sum(axis=1)         # Passer på at sumkollonnen er korrekt
        dataFrame.to_csv(livingroomPath)                                            # Lagrer filen på riktig bane

def openCurrentWeek (roomID):                                                       # Funksjon for å åpne CSV-filene som dataframes og med riktig kolonne som index
    if roomID == 1:                                                                 # Sjekker hvilket rom det er snakk om
        dataFrame = pd.read_csv(kitchenPath)                                        # Leser filen fra riktig filbane
        dataFrame = dataFrame.astype({"unix": str})                                 # Setter unix-kolonnen til å være streng
        dataFrame= dataFrame.set_index('unix')                                      # Setter unix-kolonnen til å være index
        roomMax = 2                                                                 # Definerer maksantallet som er tillatt på rommet
    if roomID == 2:                                                                 # Sjekker hvilket rom det er snakk om
        dataFrame = pd.read_csv(bathroomPath)                                       # Leser filen fra riktig filbane
        dataFrame = dataFrame.astype({"unix": str})                                 # Setter unix-kolonnen til å være streng
        dataFrame= dataFrame.set_index('unix')                                      # Setter unix-kolonnen til å være index
        roomMax = 1                                                                 # Definerer maksantallet som er tillatt på rommet
    if roomID == 3:                                                                 # Sjekker hvilket rom det er snakk om
        dataFrame = pd.read_csv(livingroomPath)                                     # Leser filen fra riktig filbane
        dataFrame = dataFrame.astype({"unix": str})                                 # Setter unix-kolonnen til å være streng
        dataFrame= dataFrame.set_index('unix')                                      # Setter unix-kolonnen til å være index
        roomMax = 3                                                                 # Definerer maksantallet som er tillatt på rommet
    return(dataFrame, roomMax)                                                      # Returnerer den åpnede filen og maksantallet på rommet

def makeNewWeek ():                                                                 # En funksjon for å lagre historiske data, samt lage nye CurrentWeek-filer

    today = getTime("date")                                                         # Henter dagens dato ved hjelp av en funksjon
    KdataFrameOld = pd.read_csv(kitchenPath)                                        # Leser av den første CurrentWeek-filen 
    KdataFrameOld.iloc[0:2016].to_csv("csv/" + str(today.year) + "-" + str(today.isocalendar()[1] - 1) + "-K.csv") # Lagrer de første 2015 linjene som historisk fil med årstall og ukenummer
    
    KdataFrameNew = makeDataFrame(KdataFrameOld)                                    # Lager en ny dataframe basert på den gamle 
    
    KdataFrameNew.to_csv(kitchenPath)                                               # Lagrer denne dataframen som den nye CSV-fila
    
    
    BdataFrameOld = pd.read_csv(bathroomPath)                                       # Leser av den første CurrentWeek-filen 
    BdataFrameOld.iloc[0:2016].to_csv("csv/" + str(today.year) + "-" + str(today.isocalendar()[1] - 1) + "-B.csv") # Lagrer de første 2015 linjene som historisk fil med årstall og ukenummer
    
    BdataFrameNew = makeDataFrame(BdataFrameOld)                                    # Lager en ny dataframe basert på den gamle 
    
    BdataFrameNew.to_csv(bathroomPath)                                              # Lagrer denne dataframen som den nye CSV-fila
    
    
    LdataFrameOld = pd.read_csv(livingroomPath)# Leser av den første CurrentWeek-filen 
    LdataFrameOld.iloc[0:2016].to_csv("csv/" + str(today.year) + "-" + str(today.isocalendar()[1] - 1) + "-L.csv") # Lagrer de første 2015 linjene som historisk fil med årstall og ukenummer
    
    LdataFrameNew = makeDataFrame(LdataFrameOld)                                    # Lager en ny dataframe basert på den gamle 
    
    LdataFrameNew.to_csv(livingroomPath)                                            # Lagrer denne dataframen som den nye CSV-fila

def makeDataFrame (dataFrameOld):                                                   # Funksjon for å generere nye dataframes basert på allerede eksisterende dataframes
    dataFrameNew = pd.DataFrame(columns=["unix", "1", "2", "3", "4", "5", "6"])     # Definerer dataframe med riktige kolonner

    weekStart = dataFrameOld.iloc[2016,0]                                           # Finner når uken starter ved hjelp av den gamle dataframen
    weekStop = weekStart + 604800 + 86400                                           # Finner ukeslutt ved å legge til en hel uke pluss en dag i sekunder
    
    betweenNumbersWeek = list(map(str, range(weekStart, weekStop,300)))             # Lager en liste som inneholder alle unix-tider vi skal fylle inn i dataframen
    
    dataFrameNew["unix"] = betweenNumbersWeek                                       # Fyller inn verdiene i unix-kolonnen
    
    column_list = list(dataFrameNew)                                                # Lager en liste som vi skal bruke til å summere bookinger
    column_list.remove("unix")                                                      # Fjerner unix-kolonnen da denne ikke skal bli med i summen
    
    dataFrameNew = dataFrameNew.set_index('unix')                                   # Setter unix-kolonnen som index
    
    dataFrameNew["sum"] = dataFrameNew[column_list].sum(axis=1)                     # Lager kolonnen sum hvor vi legger inn summen for hver rad
    
    dataFrameOld = dataFrameOld.set_index('unix')                                   # Setter unix-kolonnen til den gamle dataframen som index
    
    return(pd.concat([dataFrameOld.iloc[2016:], dataFrameNew.iloc[288:]],axis=0))   # Returner siste mandagen fra første dataframe, og alle tider fra og med tirsdag, til og med neste mandag fra den nye dataframen. Disse slått sammen til en dataframe

def checkCurrentWeekPresent ():                                                     # Funksjon for å sjekke om bookingfilene eksisterer i systemet
        
    dataFrame = pd.DataFrame(columns=["unix", "1", "2", "3", "4", "5", "6"])        # Vi lager en dataframe slik vi vil ha den

    weekStart = getMonday()                                                         # Henter ukestart med funksjonen getMonday
    weekStop = weekStart + 604800 + 86400                                           # Regner oss frem til siste ønskede tidspunkt i dataframen
    
    betweenNumbersWeek = list(map(str, range(weekStart, weekStop,300)))             # Lager en liste med alle tidspunktene vi ønsker å fylle inn
    
    dataFrame["unix"] = betweenNumbersWeek                                          # Fyller inn tidspunktene fra listen inn i unix-kolonnen
    
    column_list = list(dataFrame)                                                   # Lager en liste som vi skal bruke til å summere bookinger
    column_list.remove("unix")                                                      # Fjerner unix-kolonnen da denne ikke skal bli med i summen
    
    dataFrame = dataFrame.set_index('unix')                                         # Setter unix-kolonnen som index
    
    dataFrame["sum"] = dataFrame[column_list].sum(axis=1)                           # Lager kolonnen sum hvor vi legger inn summen for hver rad
    
    if path.exists(kitchenPath) == False:                                           # Sjekker om det finnes en fil for kjøkkenbookinger
        
        dataFrame.to_csv(kitchenPath)                                               # Hvis den ikke eksisterer lagrer vi dataframen som ny bookingfil

    if path.exists(bathroomPath) == False:                                          # Sjekker om det finnes en fil for baderomsbookinger
        
        dataFrame.to_csv(bathroomPath)                                              # Hvis den ikke eksisterer lagrer vi dataframen som ny bookingfil

    if path.exists(livingroomPath) == False:                                        # Sjekker om det finnes en fil for stuebookinger
        
        dataFrame.to_csv(livingroomPath)                                            # Hvis den ikke eksisterer lagrer vi dataframen som ny bookingfil
    
    return()                                                                        # Går ut av funksjonen

def checkForWeekChange (prevMonday):                                                # Funskjon for å sjekke om vi har gått inn i en ny uke
    if time.time() > prevMonday + 604800:                                           # Hvis nåverende tid er over en uke siden det vi har definert som forrige mandag
        makeNewWeek()                                                               # Kaller på funksjonen som lager nye booking-filer
        return("New week")                                                          # Returnerer melding om at vi er i ny uke
    else:                                                                           # Hvis det ikke er ny uke
        return("Same week")                                                         # Returnerer melding om at vi er i samme uke

