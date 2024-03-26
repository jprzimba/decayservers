CFLAGS = -I. -I/usr/include/lua5.1

LIBLINK = -L/usr/lib -lpthread -llua5.1 -lboost_thread -lgmp -lmysqlclient -lboost_regex -llua5.1-sql-mysql -llua5.1 -ldl -lboost_system

OBJ = account.o actions.o ban.o baseevents.o beds.o creature.o creatureevent.o chat.o combat.o condition.o configmanager.o connection.o container.o cylinder.o database.o databasemysql.o depot.o exception.o fileloader.o game.o globalevent.o house.o housetile.o ioguild.o iologindata.o iomap.o iomapserialize.o item.o items.o luascript.o mailbox.o map.o md5.o monster.o monsters.o movement.o networkmessage.o npc.o otserv.o outfit.o outputmessage.o party.o player.o position.o protocol.o protocolgame.o protocollogin.o quests.o raids.o rsa.o scheduler.o scriptmanager.o server.o sha1.o spawn.o spells.o status.o talkaction.o tasks.o teleport.o textlogger.o thing.o tile.o tools.o trashholder.o vocation.o waitlist.o weapons.o 

all: clean theforgottenserver

clean:
	rm -rf *.o

theforgottenserver: $(OBJ)
	g++ $(CFLAGS) -Werror -O1 -o ./TheForgottenServer $(OBJ) $(LIBLINK)

    %.o:%.cpp
	g++ $(CFLAGS) -Werror -O1 -c $+
