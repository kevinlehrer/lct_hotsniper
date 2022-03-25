UBUNTU_VERSION=16.04
DOCKER_IMAGE=ubuntu:$(UBUNTU_VERSION)-sniper-$(USER)
DOCKER_FILE=Dockerfile-ubuntu-$(UBUNTU_VERSION)

all: $(DOCKER_FILE)
	docker build -f $(DOCKER_FILE) -t $(DOCKER_IMAGE) .

run-user:
	docker run -it -v "${HOME}:${HOME}" -w "${PWD}" --user $(shell id -u):$(shell id -g) --volume="/etc/group:/etc/group:ro" --volume="/etc/passwd:/etc/passwd:ro" --volume="/etc/shadow:/etc/shadow:ro" $(DOCKER_IMAGE)

run-root:
	docker run --rm -it -v "${HOME}:${HOME}" $(DOCKER_IMAGE)
	
run:
	docker run --privileged --rm -it -v "${HOME}:${HOME}" --user $(shell id -u):$(shell id -g) -w "${PWD}" $(DOCKER_IMAGE)

.PHONY: all
