import matplotlib.pyplot as plt
import numpy as np

bocc_cta=[0.0000878,0.000088,0.000090,0.000091,0.000090,0.000088]
bocc_cta2=[0.0000893,0.000088,0.000089,0.000088,0.000091,0.000091]

focc_cta=[0.00014,0.00014,0.00015,0.00016,0.00018,0.00018]
focc_cta2=[0.00014,0.00015,0.00016,0.00016,0.00019,0.00020]

focc_ota=[0.00054,0.00054,0.00055,0.00055,0.00055,0.00057]
focc_ota2=[0.00053,0.00055,0.00054,0.00057,0.00055,0.00056]


q1_readers=[500,600,700,800,900,1000]

for i in range(6):
    bocc_cta[i] = (bocc_cta[i]+bocc_cta2[i])*500
    focc_ota[i] = (focc_ota[i]+focc_ota2[i])*500
    focc_cta[i] = (focc_cta[i]+focc_cta2[i])*500

plt.plot(q1_readers,bocc_cta,label='BOCC-CTA')
plt.plot(q1_readers,focc_ota,label='FOCC-OTA')
plt.plot(q1_readers,focc_cta,label='FOCC-CTA')
plt.legend(loc='upper left')
plt.grid()
plt.xlabel("No of transactions")
plt.ylabel("Avg commit time (millisec)")
plt.show()

bocc_cta=[8.82,8.9,8.95,8.9,9.1,9.1]
bocc_cta2=[9.00,9.1,9.17,9.2,9.3,9.4]

focc_cta=[8.93,8.77,9.04,9.12,9.2,9.4]
focc_cta2=[8.72,9.02,9.1,9.19,9.29,9.35]

focc_ota=[1.67,1.66,1.66,1.73,1.78,1.8]
focc_ota2=[1.62,1.68,1.72,1.70,1.75,1.75]


q1_readers=[500,600,700,800,900,1000]

for i in range(6):
    bocc_cta[i] = (bocc_cta[i]+bocc_cta2[i])/2
    focc_ota[i] = (focc_ota[i]+focc_ota2[i])/2
    focc_cta[i] = (focc_cta[i]+focc_cta2[i])/2

plt.plot(q1_readers,bocc_cta,label='BOCC-CTA')
plt.plot(q1_readers,focc_ota,label='FOCC-OTA')
plt.plot(q1_readers,focc_cta,label='FOCC-CTA')
plt.legend(loc='upper left')
plt.grid()
plt.xlabel("No of transactions")
plt.ylabel("Avg no of aborts")
plt.show()