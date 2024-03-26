//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

#ifndef __OTSERV_CONTAINER_H__
#define __OTSERV_CONTAINER_H__

#include "definitions.h"
#include "cylinder.h"
#include "item.h"

typedef std::list<Item *> ItemList;

class Depot;

class Container : public Item, public Cylinder
{
	public:
		Container(uint16_t _type);
		virtual ~Container();
		virtual Item* clone() const;

		virtual Container* getContainer() {return this;}
		virtual const Container* getContainer() const {return this;}
		virtual Depot* getDepot() {return NULL;}
		virtual const Depot* getDepot() const {return NULL;}

		//serialization
		bool unserializeItemNode(FileLoader& f, NODE node, PropStream& propStream);

		uint32_t size() const {return (uint32_t)itemlist.size();}
		uint32_t capacity() const {return maxSize;}

		ItemList::const_iterator getItems() const {return itemlist.begin();}
		ItemList::const_iterator getEnd() const {return itemlist.end();}

		void addItem(Item* item);
		Item* getItem(uint32_t index);
		bool isHoldingItem(const Item* item) const;

		uint32_t getItemHoldingCount() const;
		virtual double getWeight() const;

		//cylinder implementations
		virtual ReturnValue __queryAdd(int32_t index, const Thing* thing, uint32_t count,
			uint32_t flags) const;
		virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count, uint32_t& maxQueryCount,
			uint32_t flags) const;
		virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count) const;
		virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Item** destItem,
			uint32_t& flags);

		virtual void __addThing(Thing* thing);
		virtual void __addThing(int32_t index, Thing* thing);

		virtual void __updateThing(Thing* thing, uint16_t itemId, uint32_t count);
		virtual void __replaceThing(uint32_t index, Thing* thing);

		virtual void __removeThing(Thing* thing, uint32_t count);

		virtual int32_t __getIndexOfThing(const Thing* thing) const;
		virtual int32_t __getFirstIndex() const;
		virtual int32_t __getLastIndex() const;
		virtual uint32_t __getItemTypeCount(uint16_t itemId, int32_t subType = -1, bool itemCount = true) const;
		virtual Thing* __getThing(uint32_t index) const;

		virtual void postAddNotification(Thing* thing, int32_t index, cylinderlink_t link = LINK_OWNER);
		virtual void postRemoveNotification(Thing* thing, int32_t index, bool isCompleteRemoval, cylinderlink_t link = LINK_OWNER);

		virtual void __internalAddThing(Thing* thing);
		virtual void __internalAddThing(uint32_t index, Thing* thing);
		virtual void __startDecaying();

	private:
		void onAddContainerItem(Item* item);
		void onUpdateContainerItem(uint32_t index, Item* oldItem, const ItemType& oldType,
			Item* newItem, const ItemType& newType);
		void onRemoveContainerItem(uint32_t index, Item* item);

	protected:
		uint32_t maxSize;
		ItemList itemlist;
};

#endif
