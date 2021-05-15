import requests
from datetime import date,datetime
import pandas as pd
import json

def avg_price_calc():  # Funksjon som returnerer snittprisen på strøm den datoen funksjonen blir kalt
    
    today = date.today()   # Henter ut dagens dato 
    
    url = 'https://norway-power.ffail.win/?zone=NO3&date={}'.format(today) # URL til api som benyttes for å hente ut strømpriser, zone = NO3 for midtnorge,Formaterer dagens dato inn i stringens plassholder{}
    
    response = requests.get(url)     # Benytter oss av get i request bibloteket og lager oss et response objekt
    
    data = response.json()           # Konverterer dataene til JSON stringer vha json bibloteket.
    
    prices_df = pd.DataFrame.from_dict(data).transpose().drop(columns=['valid_from','valid_to'])  # Gjør om dataene fra dictionary til dataframe, deretter transpose for å gjøre det enklere å bruke dataene. Dropper deretter unødvendige kolonner
    prices_df.reset_index(level=0,inplace=True)     #Reset index for å fikse index out of bounds problem
    prices_df.columns=['Date&time','NOK']           # Endrer kolonnenavn for oversikt
    
    # prices_df['unix']=pd.to_datetime(prices_df['unix'])
    
    # for i in range(24):
    #     prices_df.at[i,'unix']=int(datetime.timestamp(prices_df.iloc[i,0]))
         
    sumdf_value = prices_df.sum(axis=0,skipna=True).iloc[1:2]  # Summerer alle prisverdiene per time i kolonne 2
    
    sumdf_value = sumdf_value[0]                               # Velger kolonne 0 for summen av strømprisene hver time dette døgnet
    
    avg_last24 = sumdf_value/24                                # Deler på 24 timer for å finne gjennomsnittet  
        
return(avg_last24)     # Returnerer snittprisen på strøm

def avg_price_upload():    # Funksjon som laster opp gjennomsnittsprisen på strøm opp i CoT
    key ="15498"            # Signalets key i Circus of Things - CoT
    token ="eyJhbGciOiJIUzI1NiJ9.eyJqdGkiOiI1MjI3In0.qmxoTz81LqxwwtambhnmU8PdpDy7LImXuhfdBYw2vq0"   # Brukers token i CoT
    value = avg_price_calc()                    #  variabel value som kaller inn verdi fra funksjonen avg_price_calc()
    
    data = {'Key':key, 'Value':value,'Token':token}  # Lager en dictionary med de ulike nødvendige verdiene key,valye og token i variabelen data
    
    requests.put('https://circusofthings.com/WriteValue',data = json.dumps(data),headers={'Content_type':'application/json'})  # Laster opp til Circus of Things


