import matplotlib.pyplot as plt
import numpy as np

bto=[0.0023,0.0018,0.0015,0.0012,0.0012]
bto2=[0.0023,0.0018,0.0015,0.0012,0.0012]

mvto=[0.00026,0.00073,0.0011,0.0016,0.0019]
mvto2=[0.00026,0.00073,0.0011,0.0016,0.0019]

q1_readers=[10,20,30,40,50]

for i in range(5):
    bto[i] = (bto[i]+bto2[i])*500
    mvto[i] = (mvto[i]+mvto2[i])*500

plt.plot(q1_readers,bto,label='BTO-CTA')
plt.plot(q1_readers,mvto,label='MVTO-OTA')
plt.legend(loc='upper right')
plt.grid()
plt.xlabel("No of transactions")
plt.ylabel("Avg commit time (millisec)")
plt.show()

bto=[3.4,4.7,6.5,9.0,11.3]
bto2=[3.4,4.7,6.5,9.0,11.3]

mvto=[4.9,6.0,7.0,7.9,8.36]
mvto2=[4.9,6.0,7.0,7.9,8.36]

q1_readers=[10,20,30,40,50]

for i in range(5):
    bto[i] = (bto[i]+bto2[i])/2
    mvto[i] = (mvto[i]+mvto2[i])/2

plt.plot(q1_readers,bto,label='BTO-CTA')
plt.plot(q1_readers,mvto,label='MVTO-OTA')
plt.legend(loc='upper left')
plt.grid()
plt.xlabel("No of transactions")
plt.ylabel("Avg no of aborts")
plt.show()