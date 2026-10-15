.PHONY: logging


vectors: logging
	cd vectors && ./build.sh

logging:
	cd logging && ./build.sh


