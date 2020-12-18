import requests
import datetime
import threading
import os

urlSite='http://192.168.1.1:1880' #hostname
urlGet='/get02'
urlParam=''
fnames='myimg.jpg'
fnameshead='myimg0_'
fnamesExt='.jpg'

WAIT_TIME_SEC = 2

myUrl = urlSite + urlGet+urlParam

def mygets(myUrl,myDir,fnameshead,fnamesExt):
	strIndx=datetime.datetime.today().strftime('%Y%m%d_%H%M%S')
	fnames = myDir+'/'+fnameshead+strIndx+fnamesExt

	with open(fnames,'wb') as fid:
		fid.write(requests.get(myUrl).content)

def mkDirEx(strDir):
	if not os.path.exists(strDir):
		os.makedirs(strDir)

ticker = threading.Event()
while not ticker.wait(WAIT_TIME_SEC):
	curDir = datetime.datetime.today().strftime('%Y%m%d')
	mkDirEx(curDir)
	mygets(myUrl,curDir,fnameshead,fnamesExt)
