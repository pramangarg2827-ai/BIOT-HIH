# SIMULATED data calibrated to readings from the user's ESP32 + ENS160 + BME280 mask (2-minute averages). Not clinical data.
# Observed: clean air TVOC ~76-88 ppb / eCO2 ~510-530 / RH ~56-57 / T ~28.2-28.6 ; normal breathing test averages TVOC 240-1500 ppb, RH 65-78 %.
import numpy as np, pandas as pd, json
from sklearn.linear_model import LogisticRegression
from sklearn.model_selection import train_test_split
from sklearn.metrics import roc_auc_score
rng=np.random.default_rng(7); n=6000
age=np.clip(rng.normal(55,12,n),18,85).round()
smoke=rng.choice([0,1,2],n,p=[.40,.25,.35])
z=0.045*(age-55)+np.array([0,.5,1.2])[smoke]
lo,hi=-8,2                      # intercept chosen so prevalence = 30% (demo balance, NOT real-world prevalence)
for _ in range(60):
    mid=(lo+hi)/2
    if (1/(1+np.exp(-(z+mid)))).mean()>.30: hi=mid
    else: lo=mid
y=(rng.random(n)<1/(1+np.exp(-(z+mid)))).astype(int)
# breath TVOC varies a lot between people/runs (observed 240-1500 for normal breathing). ASSUMPTION: +10%/+25% for former/current smokers, +50% for label=1
tvoc=380*rng.lognormal(0,.65,n)*np.array([1,1.10,1.25])[smoke]*np.where(y==1,1.5,1.0)
spike=rng.random(n)<.04                                   # sanitizer / alcohol / food contamination, unrelated to label
tvoc=np.where(spike,tvoc*rng.uniform(2,5,n),tvoc).clip(60,3000).round()
eco2=(514+177*(np.log(tvoc)-np.log(81))+rng.normal(0,25,n)).clip(400,5000).round()   # fitted to the user's 4 (TVOC,eCO2) points
rh=rng.normal(67,5,n).clip(50,92).round(1); temp=rng.normal(28.8,.9,n).clip(25,34).round(1)   # no label effect
d=pd.DataFrame(dict(session_id=range(1,n+1),age=age.astype(int),smoking=smoke,tvoc_ppb=tvoc,eco2_ppm=eco2,rh_pct=rh,temp_c=temp,label_cancer=y))
d.to_csv('synthetic_breath_sensors_v3.csv',index=False)
X=np.c_[d.age,d.smoking==1,d.smoking==2,np.log1p(d.tvoc_ppb)].astype(float)
Xtr,Xte,ytr,yte=train_test_split(X,y,test_size=.2,random_state=1,stratify=y)
mu,sd=Xtr.mean(0),Xtr.std(0); fit=lambda c:LogisticRegression(C=1e6,max_iter=3000).fit(((Xtr-mu)/sd)[:,c],ytr)
auc=lambda m,c:roc_auc_score(yte,m.predict_proba(((Xte-mu)/sd)[:,c])[:,1])
m=fit([0,1,2,3]); P=np.percentile(d.tvoc_ppb,[1,99])
print("AUC test  profile-only %.3f | tvoc-only %.3f | both %.3f"%(auc(fit([0,1,2]),[0,1,2]),auc(fit([3]),[3]),auc(m,[0,1,2,3])))
print("coef",m.coef_[0].round(3),"b",m.intercept_.round(3),"prev",y.mean().round(3),"| tvoc p1/50/99",P[0],d.tvoc_ppb.median(),P[1])
json.dump(dict(features=["age","former","current","ln_tvoc"],mean=mu.tolist(),std=sd.tolist(),coef=m.coef_[0].tolist(),intercept=float(m.intercept_[0]),
 lo=[18,0,0,float(np.log1p(P[0]))],hi=[85,1,1,float(np.log1p(P[1]))],
 metrics=dict(auc_profile=auc(fit([0,1,2]),[0,1,2]),auc_tvoc=auc(fit([3]),[3]),auc_both=auc(m,[0,1,2,3])),
 warning="Trained on SIMULATED data. Not validated. Not for medical use."),open('model_sensors.json','w'))
