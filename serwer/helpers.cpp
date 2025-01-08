#include "helpers.h"

extern std::vector<Player> players;
extern std::vector<Lobby> lobbies;

void sendToClient(int clientFd, const std::string& commandNumber, const std::string& body) {
    if (commandNumber.size() != 2) {
        std::cerr << "Error: Command number must consist of 2 characters";
        return;
    }

    std::string fullMessage = commandNumber + "\\" + body + "\n";
    ssize_t bytesSent = send(clientFd, fullMessage.c_str(), fullMessage.size(), 0);

    if (bytesSent == -1) {
        std::cerr << "Error: Failed to send message to client with socket " << clientFd << ".\n";
    }
}

void sendLobbiesToClients(std::vector<std::string> lobbyNames, int clientFd) {
    std::string messageBody;
    // przygotowanie wiadomości
    for (size_t i = 0; i < lobbyNames.size(); ++i) {
        messageBody += lobbyNames[i];
        if (i != lobbyNames.size() - 1) {
            messageBody += ",";
        }
    }
    // wysłanie do jednego klienta (który dopiero włączył aplikację)
    if (clientFd != -1) {
        sendToClient(clientFd, "70", messageBody);
    }
    // wysłanie do wielu klientów (update dla klientów, którzy są już w aplikacji)
    else {
        for (const auto& player : players) {
            if (player.lobbyName.empty()) {
                sendToClient(player.sockfd, "70", messageBody);
            }
        }
    }
}

void sendPlayersToClients(const Lobby* lobby, int ignoreFd) {
    std::string msgBody;

    // przygotowanie wiadomości
    for (size_t i = 0; i < lobby->players.size(); ++i) {
        if (lobby->players[i].sockfd != ignoreFd) {
            msgBody += lobby->players[i].nick;
            if (i != lobby->players.size() - 1) {
                msgBody += ",";
            }
        }
    }

    // wysłanie do wszystkich klientów z pokoju
    for (const auto& player : lobby->players) {
        if (player.sockfd != ignoreFd) {
            sendToClient(player.sockfd, "71", msgBody);
        }
    }
}

void sendGameUpdatesToClients(const Lobby* lobby) {
    std::string msgWord = lobby->game.wordInProgress;
    std::string msgLives;
    std::string msgPoints;

    for (size_t i = 0; i < lobby->players.size(); ++i) {
        msgLives += lobby->players[i].nick + ":" + std::to_string(lobby->players[i].lives);
        msgPoints += lobby->players[i].nick + ":" + std::to_string(lobby->players[i].points);
        if (i != lobby->players.size() - 1) {
            msgLives += ",";
            msgPoints += ",";
        }
    }

    // wysłanie do wszystkich klientów z pokoju
    for (const auto& player : lobby->players) {
        sendToClient(player.sockfd, "75", msgWord);
        sendToClient(player.sockfd, "76", msgLives);
        sendToClient(player.sockfd, "77", msgPoints);
    }
}

void isStartAllowed(const Lobby* lobby) {
    if (!lobby->game.isGameActive) {
        if (lobby->playersCount >= 2) {
            sendToClient(lobby->players[0].sockfd, "72", "1");
        }
        else {
            sendToClient(lobby->players[0].sockfd, "72", "0");
        }
    }
}

Settings parseSettings(std::string msg) {
    Settings settings;

    size_t posName = msg.find("name:");
    size_t posPass = msg.find("password:");
    size_t posDiff = msg.find("difficulty:");
    size_t posRounds = msg.find("rounds:");
    size_t posTime = msg.find("time:");

    size_t nameStart = posName + strlen("name:"); // po "name:"
    size_t nameEnd = msg.find(",", nameStart); // do przecinka
    settings.name = msg.substr(nameStart, nameEnd - nameStart);

    size_t passStart = posPass + strlen("password:");
    size_t passEnd = msg.find(",", passStart);
    settings.password = msg.substr(passStart, passEnd - passStart);

    size_t diffStart = posDiff + strlen("difficulty:");
    size_t diffEnd = msg.find(",", diffStart);
    settings.difficulty = std::stoi(msg.substr(diffStart, diffEnd - diffStart));

    size_t roundsStart = posRounds + strlen("rounds:");
    size_t roundsEnd = msg.find(",", roundsStart);
    settings.roundsAmount = std::stoi(msg.substr(roundsStart, roundsEnd - roundsStart));

    size_t timeStart = posTime + strlen("time:");
    size_t timeEnd = msg.find(",", timeStart);
    settings.roundDurationSec = std::stoi(msg.substr(timeStart, timeEnd - timeStart));

    return settings;
}

std::string messageSubstring(std::string msg) {
    return msg.substr(3);
}

void removeFromLobby(int clientFd) {
    for (auto & lobby : lobbies) {
        auto playerIt = std::find_if(lobby.players.begin(), lobby.players.end(), [clientFd](const Player& player) {
            return player.sockfd == clientFd;
        });

        if (playerIt != lobby.players.end()) {
            std::cout << "Player: " << playerIt->nick << ", got removed from lobby: " << lobby.name << "\n";
            lobby.playersCount--;
            if (lobby.game.isGameActive) { // gra trwa (game page)
                for (auto & player : lobby.players) {
                    if (playerIt->nick != player.nick) {
                        sendToClient(player.sockfd, "74", playerIt->nick);
                    }
                }
            }
            else { // gra nie trwa (waitroom lub end page)
                sendPlayersToClients(&lobby, playerIt->sockfd);
                isStartAllowed(&lobby);
            }
            lobby.players.erase(playerIt);
            break;
        }
    }
}
