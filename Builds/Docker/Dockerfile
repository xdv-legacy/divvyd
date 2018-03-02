# divvyd

# use the ubuntu base image
FROM ubuntu
MAINTAINER Roberto Catini roberto.catini@gmail.com

# make sure the package repository is up to date
RUN apt-get update
RUN apt-get -y upgrade

# install the dependencies
RUN apt-get -y install git scons pkg-config protobuf-compiler libprotobuf-dev libssl-dev libboost1.55-all-dev

# download source code from official repository
RUN git clone https://github.com/xdv/divvyd.git src; cd src/; git checkout master

# compile
RUN cd src/; scons build/divvyd

# move to root directory and strip
RUN cp src/build/divvyd divvyd; strip divvyd

# copy default config
RUN cp src/doc/divvyd-example.cfg divvyd.cfg

# clean source
RUN rm -r src

# launch divvyd when launching the container
ENTRYPOINT ./divvyd
