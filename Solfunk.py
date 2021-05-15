import requests
from datetime import datetime, date
import pandas as pd
import hentvaerdata as hv
import json


# // Ble ikke tatt i bruk 
def sunrise():      # Funksjon som returnerer tidspunkt for soloppgang i Trondheim , dagen man kaller funksjonen // Ble ikke brukt, da vi valgte en annen løsning
    url = 'http://api.openweathermap.org/data/2.5/weather?q=Trondheim&appid=a1cb393be9973b540ce25cc8c7e948ad&units=metric' #url til API'en som ble brukt til å hente ut disse data, med en q=Trondheim, appid/key og units=metric
    res = requests.get(url)  # Benytter oss av get i request bibloteket og lager oss et response objekt
    data = res.json()        # Konverterer dataene til JSON stringer vha json bibloteket.
    sunrise_unixtime = data['sys']['sunrise'] # Velger hvor vi vil inn i de forskjellige json stringene, først inn i "sys" som er en dictionary, deretter inn i "sunrise" som er en int unix verdi
    sunrise_time = datetime.fromtimestamp(int(sunrise_unixtime)).strftime('%Y-%m-%d %H:%M:%S') # Konverterer fra Unix-verdi til en verdi som er lesbar i %Y-%m-%d %H:%M:%S' format
    return (sunrise_time) # returnerer tidspunkt





 # // Ble ikke tatt i bruk 
def sunset():      # Funksjon soom returnerer tidspunkt for solnedgang i Trondhei, dagen man kaller funksjonen
    url ='http://api.openweathermap.org/data/2.5/weather?q=Trondheim&appid=a1cb393be9973b540ce25cc8c7e948ad&units=metric' #url til API'en som ble brukt til å hente ut disse data, med en q=Trondheim, appid/key og units=metric
    res2 = requests.get(url)  # Benytter oss av get i request bibloteket og lager oss et response objekt
    data2 = res2.json()       # Konverterer dataene til JSON stringer vha json bibloteket.
    sunset_unixtime = data2['sys']['sunset'] # Velger hvor vi vil inn i de forskjellige json stringene, først inn i "sys" som er en dictionary, deretter inn i "sunrise" som er en int unix verdi
    sunset_time = datetime.datetime.fromtimestamp(sunset_unixtime).strftime('%Y-%m-%d %H:%M:%S')  # Konverterer fra Unix-verdi til en verdi som er lesbar i %Y-%m-%d %H:%M:%S' format
    return (sunset_time) # returnerer tidspunkt

# // Ble ikke tatt i bruk 
def solarnoon():            # Funksjon som returnerer tidspunkt i den dagen funksjonen blir kalt tidspunktet der sola står høyest på dagen // Ble ikke brukt, da vi valgte en annen løsning
    today = date.today()    # Henter ut dagens dato mved hjelp av date.today()
    url = 'https://api.met.no/weatherapi/sunrise/2.0/.json?date={}&lat=63.411990&lon=10.399606&offset=02%3A00'.format(today) # Meteorologisk institutt sin API for å hente alskens værdata, Formaterer dagens dato inn i stringens plassholder{}, og med Latitude&longitude verdier = Trondheim
    identifier = {"user-agent" : "markuhse@stud.ntnu.no"}   # Identifier for å få tilgang til API'en
        
    response = requests.get(url, headers=identifier)  # Requester API'en med urlen og identifieren som header
    data = response.json()        # Konverterer dataene til JSON stringer vha json bibloteket.
    data = data['location']['time'][0]['solarnoon']['time']  # Henter ut at det er solar noon time vi vil ha
    str_time = data       # Gjør om data til variabelen str_time for å vise at data er en string
    solarnoon_time = datetime.timestamp(datetime.strptime(str_time,"%Y-%m-%dT%H:%M:%S+02:00")) # Konverterer fra string til datetime objekt
    
    return (solarnoon_time) # Returnerer tidspunkt hvor sola står høyest




# // Ble ikke tatt i bruk 
def noon_angle():                   # Funksjon som returnerer vinkel på sola i det sola står høyest  // Ble ikke brukt, da vi valgte en annen løsning
    today = date.today()            # Henter ut dagens dato mved hjelp av date.today()
    url = 'https://api.met.no/weatherapi/sunrise/2.0/.json?date={}&lat=63.411990&lon=10.399606&offset=02%3A00'.format(today)  # Meteorologisk institutt sin API for å hente alskens værdata, Formaterer dagens dato inn i stringens plassholder{}, og med Latitude&longitude verdier = Trondheim
    identifier = {"user-agent" : "markuhse@stud.ntnu.no"}           # Identifier for å få tilgang til API'en
    response = requests.get(url, headers=identifier)                # Requester API'en med urlen og identifieren som header
    angle_data = response.json()                                    # Konverterer dataene til JSON stringer vha json bibloteket.
    noon_angle = angle_data['location']['time'][0]['solarnoon']['elevation']  # Går inn i de ulike typene og henter ut elevation, som sier oss float av vinkelen til sola
    return (noon_angle) # Returnerer vinkelen til sola 





def H_radiation():      # Funksjon som returnerer total stråling siste 24 timer [W/m^2] // Eneste i bruk
    url = 'http://api.solcast.com.au/world_radiation/estimated_actuals?latitude=63.4249408&longitude=10.430212100000002&hours=24&api_key=0_yShsxWvP0r4ZaA6YNGI3S1KIbb2XhV&format=json'
    #Url til solcast sin API som leverer estimerte strålingsverdier fra hvor som helst i verden, latitude&longituide tilpasset Trondheim. stråling = siste 24 hours, oppdateres hver halvtime. &apikey man får ved å lage en bruker. format=json
    
    
    response = requests.get(url)                # Benytter oss av get i request bibloteket og lager oss et response objekt
    data = response.json()                      # Konverterer dataene til JSON stringer vha json bibloteket.
    
    ghi_0 = data['estimated_actuals']           #Inn i kategorien 'estimated_actuals'
    radiation_df = pd.DataFrame.from_dict(ghi_0).drop(columns=['ebh','dni','dhi','cloud_opacity','period'])  # Gjør om fra dictionary til dataframe vha pandas, dropper deretter kolonnene vi ikke trenger
    radiation_df['period_end']=pd.to_datetime(radiation_df['period_end'])                                    # Gjør om kolonnen 'period_end' til datetime objekt 
    radiation_df.set_index('period_end',inplace = True)                                                      # Setter nå denne 'period_end' til indeks for bedre oversikt
    
    
    Ghi_sum = radiation_df.iloc[0:48].sum()             # Summerer stråling siste 24 timer [Ghi]
    
    Ghi_value = Ghi_sum.iloc[0]                         # Velger oss rad 0 for å kunne hente ut verdien
        
    return (Ghi_value)        # Returnerer total stråling estimert siste 24 timer











