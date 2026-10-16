.PHONY: logging


vectors: logging
	cd vectors && ./build.sh

allocation: logging
	cd allocation && ./build.sh

logging:
	cd logging && ./build.sh

data_structures:
	cd data_structures && ./build.sh


