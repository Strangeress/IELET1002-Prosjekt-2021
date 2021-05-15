import requests
import json


def vaerdata_temp():  # Funksjon for å hente ut live temperaturen i Trondheim
    url = " "  # Url'en til API'en som benyttes for å hente ut live temperatur i Trondheim
    
    identifier = {"user-agent" : "markuhse@stud.ntnu.no"}                                  # Identifier for å få tilgang til API'en
    
    response = requests.get(url, headers=identifier)                                       # Requester API'en med urlen og identifieren som header
    
    temp = response.json()                          # Konverterer dataene til JSON stringer vha json bibloteket.
    
    temp = temp['properties']['timeseries'][0]['data']['instant']['details']['air_temperature'] # Går inn i ulike kategorier og henter ut 'Air_temperature', siden det er den vi er ute etter
  
   
    return temp   # Returner temperaturen akkurat nå i Trondheim



def temp_upload():  # Funksjon som laster opp temperaturen både opp i room registeret i CoT (roomkey), og smartmeteret(smartkey)
    # roomkey ="10893"
    smartkey = "3183"       # Signalets key i Circus of Things - CoT
    token ="eyJhbGciOiJIUzI1NiJ9.eyJqdGkiOiI1MjI3In0.qmxoTz81LqxwwtambhnmU8PdpDy7LImXuhfdBYw2vq0"     # Brukers token i CoT
    
    # response = requests.get("https://circusofthings.com/ReadValue?Key=" + roomkey + "&Token=" + token)
    
    # data=json.loads(response.content)
    # value = str(data["Value"])
    
  
    temp = vaerdata_temp()      # Lagrer temperaturverdien i variabelen temp (str)
    
    # tempStr = "%(number)03d" % {"number": (int(temp)+100)}
    
    # value = value[0:4] + tempStr + value[7:]
    
    # requests.get("https://circusofthings.com/WriteValue?Key=" + roomkey + "&Value=" + value + "&Token=" + token)
    requests.get("https://circusofthings.com/WriteValue?Key=" + smartkey + "&Value=" + str(temp) + "&Token=" + token)    # requester CoT og legger til smartkey,value og token
  






def vaerdata_clouds(): # Funksjon som returnerer skydekke [0-100] // Ikke i bruk
    url = "https://api.met.no/weatherapi/locationforecast/2.0/compact?lat=63.411990&lon=10.399606"  # Url'en til API'en som benyttes
    
    identifier = {"user-agent" : "markuhse@stud.ntnu.no"}            # Identifier for å få tilgang til API'en
    
    response = requests.get(url, headers=identifier)                 # Requester API'en med urlen og identifieren som header
    
    clouds = response.json()                                        # Konverterer dataene til JSON stringer vha json bibloteket.
    
    clouds = clouds['properties']['timeseries'][0]['data']['instant']['details']['cloud_area_fraction']   # Henter ut 'cloud_area_fraction'
    
    return clouds  # returnerer verdien på skydekke

def cloud_upload():   # Funksjon for å laste opp skydekke verdi til Circus of Things
    key ="23091"      # Signalets key i Circus of Things - CoT
    token ="eyJhbGciOiJIUzI1NiJ9.eyJqdGkiOiI1MjI3In0.qmxoTz81LqxwwtambhnmU8PdpDy7LImXuhfdBYw2vq0"  # Brukers token i CoT
    value = vaerdata_clouds() # Value som skal lastes opp, kaller på funksjonen vaerdata_clouds()
    
    data = {'Key':key, 'Value':value,'Token':token} # Lager en dictionary med de ulike nødvendige verdiene; key,valye og token i variabelen data
    
    requests.put('https://circusofthings.com/WriteValue',data = json.dumps(data),headers={'Content_type':'application/json'})  # "Dumper" inn dictionarien og laster opp til CoT ??











def vaerdata_wind():
    url = "https://api.met.no/weatherapi/locationforecast/2.0/compact?lat=63.411990&lon=10.399606"
    
    identifier = {"user-agent" : "markuhse@stud.ntnu.no"}
    
    response = requests.get(url, headers=identifier)
    
    windspeed = response.json()
    windspeed = windspeed['properties']['timeseries'][0]['data']['instant']['details']['wind_speed']
    
    return windspeed

def wind_upload():
    key = "24913"
    token ="eyJhbGciOiJIUzI1NiJ9.eyJqdGkiOiI1MjI3In0.qmxoTz81LqxwwtambhnmU8PdpDy7LImXuhfdBYw2vq0"
    value = vaerdata_wind()
    
    data = {'Key':key, 'Value':value,'Token':token}    
    
    requests.put('https://circusofthings.com/WriteValue',data = json.dumps(data),headers={'Content_type':'application/json'})  
    



