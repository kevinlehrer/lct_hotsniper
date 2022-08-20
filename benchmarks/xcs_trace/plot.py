import matplotlib.pyplot as plt
import numpy as np
import os
import pathlib

directory = os.path.dirname(os.path.abspath(__file__))
plt.figure(figsize=(14,10))
plt.figure(figsize=(14,10))

for idx, filepath in enumerate(os.listdir(directory)):
	f = os.path.join(directory, filepath)
	filename = os.path.basename(filepath)
	# checking if it is a file
	if os.path.isfile(f) and os.stat(f).st_size > 0 and  pathlib.Path(f).suffix == '.log':
		filename = os.path.basename(filepath)
		print(filename)
		reward = np.genfromtxt(filename, delimiter=",", usemask=True)
		if reward.sum() != 0:
			plt.plot(reward, label=filename)
#plt.ylim(1)
plt.grid()
plt.grid(which='minor', linestyle=':')
plt.minorticks_on()
plt.title(label='LCT reward')
plt.legend()
plt.savefig('LCT_reward.png', bbox_inches='tight')
plt.close()
