from Solfunk import H_radiation # Importerer strålingsverdier fra funksjonen H_Radiation() i solfunk filen
import hentvaerdata as hv      # Importerer hentvaerdata som hv
import requests
import json



def Temperature_loss():          # Funksjon for temperatur tap koeffisient, veldig lav temperatur-> tap i energiproduksjon. Veldig varm temp -> tap i energiproduksjon

    Loss_max = float(0.15)      # Maksimalt tap
    Loss_min = float(0.03)      # Minimalt tap
    Temp = hv.vaerdata_temp()
    
    if ( Temp < -25):         # Henter funksjonen vaerdata_temp fra modulen hv. Denne funksjonen returnerer temperatur i Trondheim akkurat nå
        Temp_loss = Loss_max                # Hvis temperaturen er mindre enn -25, temperatur tap = maks
    elif (-25 < Temp <= -15):  
        Temp_loss = float(0.12)             # Hvis temperatur er mellom -25 og -15, tap på 0.12
    elif (-15 < Temp <= -5):
        Temp_loss = float(0.08)             # Hvis mellom -15 og -5, tap på 0.08 
    elif (-5 < Temp <= 5):
        Temp_loss = Loss_min                # Hvis temperatur er mellopm -5 og 5 grader, er tapet minimalt.
    elif (5 < Temp <= 15):
        Temp_loss = 0.08                    # Hvis temperatur er mellom 5 og 15 grader, er tapet på 0.08
    elif (15 < Temp <= 25):
        Temp_loss = 0.12                    # Hvis temperatur er mellom 15 og 25 grader, er tapet på 0.12
    elif (Temp > 25):
        Temp_loss = Loss_max                # Hvis temperatur er over 25 varmegrader, så er tapet maksimalt
    else:
        Temp_loss = 0
               
    
    return Temp_loss                        # Returnerer Temp_loss, koeffisient for tap


                        


def daily_Solarproduction():        # Funksjon som returnerer prudsert strøm fra solcelleanlegget siste 24 timer [kWh]
    Inverter_losses = float(0.08)    # 8 % tap
    temp_loss = Temperature_loss()   # temp_loss avhengig av funksjonen Temperature_loss() -> angir tapet avhengig av nåværende temperatur
    cable_loss = float(0.04)         # 4 % tap gjennom kabler
    Shadings = float(0.03)           # 3 % tap skygge
    dust_snow_etc = float(0.02)      # 4 % tap støv, snø, regn 
    
    details = Inverter_losses+temp_loss+cable_loss+Shadings+dust_snow_etc   # summerer alle konstanter, blant annet temperatur tap. 
    
    A = int(56)                        # A = Areal solcelleanlegg [m2]
    r = float(0.19)                    # r = Solcelleanleggets effektivitet, oppgitt i datablad 19%                                     
    H = H_radiation()/1000             # H = Henter inn strålingsverdiene i Trondheim siste 24 timer, deler på 1000 for å gjøre om til kWh per m2
    Pr = float(1-details)              # Pr = Ytelsesforhold, ulike konstanter der det vil bli tap. Invertere, temperaturavhenig, kabler, evt skygge, støv,snø,regn
    
    Daily = A*r*H*Pr                # Global formel for å estimere produksjonen til et solcelleanlegg
    return Daily                    # Returnerer samlet produksjon siste 24 timer. Tanken er å kjøre denne når hver dag er over sånn at man får opp dagens samlede produksjon



def solarMain():        # Funksjon som laster opp samlet produksjon siste 24 timer til Circus of Things, returnerer også verdien 
    key ="75"           # Signalets key i Circus of Things - CoT
    token ="eyJhbGciOiJIUzI1NiJ9.eyJqdGkiOiI1MjI3In0.qmxoTz81LqxwwtambhnmU8PdpDy7LImXuhfdBYw2vq0"           # Brukers token i CoT
    value = daily_Solarproduction()             #  variabel value som kaller inn verdi fra funksjonen daily_Solarproduction()
    
    data = {'Key':key, 'Value':value,'Token':token}         # Lager en dictionary med de ulike nødvendige verdiene key,valye og token i variabelen data
    
    requests.put('https://circusofthings.com/WriteValue',data = json.dumps(data),headers={'Content_type':'application/json'}) # Laster opp til Circus of Things
    
    return(value)







    














    

    
    