FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update
RUN apt-get -y install sudo build-essential apt-file curl less wget cmake git vim cmake-curses-gui python3

#RUN bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)"
RUN apt-get update
RUN apt-file update

#RUN apt-get -y install libclang-9-dev llvm-9-dev clang-9
#RUN update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-9 60
#RUN update-alternatives --install /usr/bin/clang++ c++ /usr/bin/clang++-9 60
#RUN update-alternatives --install /usr/bin/llvm-config llvm-config /usr/bin/llvm-config-9 60

RUN apt-get -y install libsqlite3-dev

RUN cd /usr/src && \
	wget https://github.com/CLIUtils/CLI11/archive/v1.9.0.tar.gz && \
	tar xvf v1.9.0.tar.gz && \
	cd CLI11-1.9.0 && \
	mkdir build && \
	cd build && \
	cmake .. -DCLI11_BUILD_TESTS=0 && \
	make -j$(nproc) && \
	make install

RUN useradd -m cpp2sqlite && echo "cpp2sqlite:cpp2sqlite" | chpasswd && adduser cpp2sqlite sudo
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

USER cpp2sqlite
RUN mkdir -vp /home/cpp2sqlite/cpp2sqlite
COPY . /home/cpp2sqlite/cpp2sqlite

#RUN cd /home/cpp2sqlite && \
#	wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/llvm-project-10.0.0.tar.xz && \
#	cd llvm-project-10.0.0 && \

ENTRYPOINT /bin/bash
