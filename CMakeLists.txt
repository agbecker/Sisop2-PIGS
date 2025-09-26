cmake_minimum_required(VERSION 3.10)

# nome do projeto
project(Sisop2-PIGS)

# define a versão de C++ (aqui C++17, mas pode ser 20/23)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# cria o executável com todos os arquivos .cpp
add_executable(pigs
    src/server/Server.cpp
    src/server/Interface.h
    src/server/Interface.cpp
    src/server/Discovery.h
    src/server/Discovery.cpp
)

# no Linux é comum precisar linkar a biblioteca de threads
find_package(Threads REQUIRED)
target_link_libraries(pigs PRIVATE Threads::Threads)
