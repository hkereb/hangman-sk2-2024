#include "handle-client-message.h"
#include "network.h"
#include "helpers.h"

#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>

#include <iostream>
#include <list>

std::vector<Player> players;
std::vector<std::string> playersNicknames;

std::list<Lobby> gameLobbies;
int lobbyCount = 0;
std::vector<std::string> lobbyNames;

int main(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "no extra arguments required\n");
        return 1;
    }

    int sockfd = startListening();

    int efd = epoll_create(MAXEPOLLSIZE);

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;

    if (epoll_ctl(efd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        perror("epoll_ctl");
        return -1;
    } else {
        printf("success insert listening socket into epoll.\n");
    }

    int fdsToWatch = 1;
    struct epoll_event *events;
    events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * fdsToWatch);

    std::unordered_map<int, std::string> clientBuffers;

    while (1) {  // loop for accepting incoming connections
        int ready = epoll_wait(efd, events, fdsToWatch, -1);
        if (ready == -1) {
            perror("epoll_wait");
            break;
        }

        for (int n = 0; n < ready; ++n) {
            if (events[n].data.fd == sockfd) {
                // std::cout << "sockfd" << std::endl;
                sockaddr_in clientAddr{};
                socklen_t addrSize = sizeof(clientAddr);
                int newFd = accept(events[n].data.fd, (struct sockaddr *)&clientAddr, &addrSize);
                if (newFd == -1) {
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                        break;
                    } else {
                        perror("accept");
                        break;
                    }
                }

                printf("server: new connection established.\n");
                setNonBlocking(newFd);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = newFd;
                if (epoll_ctl(efd, EPOLL_CTL_ADD, newFd, &ev) < 0) {
                    printf("Failed to insert socket into epoll.\n");
                }
                fdsToWatch++;

                Player newPlayer;
                newPlayer.sockfd = newFd;
                players.push_back(newPlayer);

                sendToClient(newFd, "69", "hello!");
            }
            else {
                //std::cout << "clientfd" << std::endl;
                while (true) {
                    char buffer[1024] = {0};
                    int bytesReceived = recv(events[n].data.fd, buffer, sizeof(buffer) - 1, 0);
                    if (bytesReceived <= 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        // client disconnected, remove player
                        perror("recv (wiadomość od klienta)");

                        // todo musi pójść wiadomość do graczy z tego lobby z aktualizacją listy graczy w pokoju (funkcja już jest) (jeżeli isActive=False)
                        // todo wiadomość do graczy w trwającej rozgrywce o tym że konkretny gracz [nickname] został usunięty z ich gry
                        // todo trzeba zmniejszyć players_count w lobby o -1
                        // todo trzeba ponownie sprawdzić czy gra może zostać rozpoczęta (jeżeli isActive = false) i wysłać wiadomość (funkcja już jest)
                        // todo rozesłać na nowo listę pokoi do graczy którzy nie są w żadnym lobby (funkcja już jest)
                        // search for player and remove them from lobby
                        for (auto& lobby : gameLobbies) {
                            auto playerIt = std::find_if(lobby.players.begin(), lobby.players.end(), [n, events, &lobby](const Player& player) {
                                return player.sockfd == events[n].data.fd;
                            });

                            if (playerIt != lobby.players.end()) {
                                lobby.players.erase(playerIt);  // removed
                                std::cout << "Player with sockfd " << events[n].data.fd << " removed from lobby: " << lobby.name << "\n";
                                break;
                            }
                        }

                        // remove player from global list
                        auto playerIt = std::find_if(players.begin(), players.end(), [n, events](const Player& player) {
                            return player.sockfd == events[n].data.fd;
                        });

                        if (playerIt != players.end()) {
                            players.erase(playerIt); 
                            std::cout << "Player with sockfd " << events[n].data.fd << " removed from global player list.\n";
                            
                            auto nicknameIt = std::find(playersNicknames.begin(), playersNicknames.end(), playerIt->nick);
                            if (nicknameIt != playersNicknames.end()) {
                                playersNicknames.erase(nicknameIt);  // Remove the nickname
                                std::cout << "Player's nickname removed: " << playerIt->nick << "\n";
                            }
                        }

                        // Clean up resources
                        close(events[n].data.fd);
                        epoll_ctl(efd, EPOLL_CTL_DEL, events[n].data.fd, &ev);
                        fdsToWatch--;
                        break;
                    }
                    clientBuffers[events[n].data.fd] += std::string(buffer, bytesReceived);

                    std::string& clientBuffer = clientBuffers[events[n].data.fd];
                    size_t pos;
                    while ((pos = clientBuffer.find('\n')) != std::string::npos) {
                        std::string clientMessage = clientBuffer.substr(0, pos);
                        clientBuffer.erase(0, pos + 1);

                        std::string outputMessage = "klient: " + clientMessage + "\n";
                        write(1, outputMessage.c_str(), outputMessage.size());

                        handleClientMessage(events[n].data.fd, clientMessage);
                    }
                }
            }
        }
    }

    free(events);
    close(sockfd);
    return 0;
}