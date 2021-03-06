#ifndef __RELATIONS_H__
#define __RELATIONS_H__

#include "base/Base.h"
#include "FairyGUIMacros.h"
#include "RelationItem.h"

NS_FGUI_BEGIN

class GObject;

class Relations : public Ref
{
    FUI_RTTI_DERIVED(Relations, Ref)
public:
    Relations();
    Relations(GObject* owner);
    ~Relations();

    CREATE_FUNC(Relations);

    void add(GObject* target, RelationType relationType);
    void add(GObject* target, RelationType relationType, bool usePercent);
    void addItems(GObject* target, const char* sidePairs);
    void remove(GObject* target, RelationType relationType);
    bool contains(GObject* target);
    void clearFor(GObject* target);
    void clearAll();
    void copyFrom(const Relations& source);
    void onOwnerSizeChanged(float dWidth, float dHeight);
    bool isEmpty() const;
    void setup(TXMLElement* xml);

    GObject* handling;

    GObject* _owner;
private:
    bool init() { return true; }

    vector<RelationItem*> _items;
};

NS_FGUI_END

#endif
