# Payment via Internet Guarantee System - PIGS

```
                  $                                               
                 $$$                                              
                $ $ $                                             
                $ $                                               
                 $$                                               
                  $$                                              
                  $ $                                             
                $ $ $                  #####   ####  ####    #### 
            -----$$$---xxxxx           ##   #   ##  ##   #  #    #
      \ xxx-      $         xxx        ##   #   ##  ##       ##   
     | \       =======         xx      #####    ##  ##  ##     ## 
     x |     -                   x     ##       ##  ##   #  #    #
    x  0    / |                   x    ##      ####  ####    #### 
  oooo    0   |                   x                               
 o    o                           x    Payment via                
o |  | o                          x                               
o |  | o                          x    Internet                   
o |  | o                         x                                
 o    o                        xxv     Guarantee                  
  oooo                      xxx  v                                
  v  vxx               xxxxx v   v     System                     
  v v   xxxxxxxxxxxxxxx       v  v                                
  vv     v    v                vv                                 
          v  v                                                    
           vv                                                                                                     
```

Trabalho desenvolvido para a disciplina INF01151 - Sistemas Operacionais II

Professor: Eder John Scheid

Integrantes:
- Álvaro Guglielmin Becker
- Arthur Henrique Lutz Amaral
- Felipe Nunes Laguna
- Gabriel Ricci Pazinatto

---
# Organização do grupo

## Estrutura básica do programa

**Cliente**: A princípio não tem por que rodar múltiplas threads. Ao inicializar, faz o processo de descoberta e armazena o IP do servidor. 

Depois, para cada input do usuário, espera até receber resposta do servidor (ou dar timeout). Daí bloqueia a entrada até imprimir o resultado da requisição, e permite o próximo input.

--

**Servidor**:
- Thread principal `Server`, que articula as demais threads e armazena a lista de clientes
- `Interface` é uma thread única, que cuida uma fila de eventos. Se a fila não estiver vazia, trava ela, remove o evento na cabeça, destrava, e imprime seu status corretamente.
- `Discovery` tem uma thread principal que cuida o socket. Quando recebe uma mensagem, cria uma nova thread e passa o IP para ela. A nova thread bloqueia a lista de usuários, adiciona um novo usuário a ela, desbloqueia, e envia a resposta com o IP ao usuário.
- `Process` tem uma thread principal que cuida o socket. Quando recebe uma mensagem, cria uma nova thread e passa a mensagem para ela. A nova thread valida a mensagem, trava os usuários relevantes na lista, tenta efetuar a transferência, destrava e manda mensagem com o resultado ao usuário que pediu.