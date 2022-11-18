FROM gcc:9.4

COPY . /usr/src/myapp

WORKDIR /usr/src/myapp

# RUN gcc -g Part\ a/TestTask2.c Part\ a/thread.c -o Tests/test2

# CMD ["Tests/test2"]
