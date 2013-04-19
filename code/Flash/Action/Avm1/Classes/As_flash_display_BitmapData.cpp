#include "Flash/Action/ActionFunctionNative.h"
#include "Flash/Action/Avm1/Classes/As_flash_display_BitmapData.h"
#include "Flash/Action/Classes/BitmapData.h"

namespace traktor
{
	namespace flash
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.flash.As_flash_display_BitmapData", As_flash_display_BitmapData, ActionClass)

As_flash_display_BitmapData::As_flash_display_BitmapData(ActionContext* context)
:	ActionClass(context, "flash.display.BitmapData")
{
	Ref< ActionObject > prototype = new ActionObject(context);

	prototype->addProperty("height", createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_get_height), 0);
	prototype->addProperty("rectangle", createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_get_rectangle), 0);
	prototype->addProperty("transparent", createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_get_transparent), 0);
	prototype->addProperty("width", createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_get_width), 0);

	//prototype->setMember("applyFilter", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_applyFilter)));
	//prototype->setMember("clone", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_clone)));
	//prototype->setMember("colorTransform", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_colorTransform)));
	//prototype->setMember("copyChannel", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_copyChannel)));
	//prototype->setMember("copyPixels", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_copyPixels)));
	//prototype->setMember("dispose", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_dispose)));
	//prototype->setMember("draw", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_draw)));
	//prototype->setMember("fillRect", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_fillRect)));
	//prototype->setMember("floodFill", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_floodFill)));
	//prototype->setMember("generateFilterRect", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_generateFilterRect)));
	//prototype->setMember("getColorBoundsRect", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_getColorBoundsRect)));
	//prototype->setMember("getPixel", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_getPixel)));
	//prototype->setMember("getPixel32", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_getPixel32)));
	//prototype->setMember("hitTest", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_hitTest)));
	//prototype->setMember("loadBitmap", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_loadBitmap)));
	//prototype->setMember("merge", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_merge)));
	//prototype->setMember("noise", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_noise)));
	//prototype->setMember("paletteMap", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_paletteMap)));
	//prototype->setMember("perlinNoise", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_perlinNoise)));
	//prototype->setMember("pixelDissolve", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_pixelDissolve)));
	//prototype->setMember("scroll", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_scroll)));
	//prototype->setMember("setPixel", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_setPixel)));
	//prototype->setMember("setPixel32", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_setPixel32)));
	//prototype->setMember("threshold", ActionValue(createNativeFunction(context, this, &As_flash_display_BitmapData::BitmapData_threshold)));

	prototype->setMember("constructor", ActionValue(this));
	prototype->setReadOnly();

	setMember("prototype", ActionValue(prototype));
}

void As_flash_display_BitmapData::initialize(ActionObject* self)
{
}

void As_flash_display_BitmapData::construct(ActionObject* self, const ActionValueArray& args)
{
	Ref< BitmapData > bm;
	if (args.size() >= 2)
	{
		bool transparent = false;
		uint32_t fillColor = 0x00000000;

		if (args.size() >= 3)
			transparent = args[2].getBoolean();

		if (args.size() >= 4)
			fillColor = uint32_t(args[3].getNumber());

		bm = new BitmapData(
			int32_t(args[0].getNumber()),
			int32_t(args[1].getNumber()),
			transparent,
			fillColor
		);
	}
	self->setRelay(bm);
}

ActionValue As_flash_display_BitmapData::xplicit(const ActionValueArray& args)
{
	return ActionValue();
}

avm_number_t As_flash_display_BitmapData::BitmapData_get_height(const BitmapData* self) const
{
	return avm_number_t(self->getWidth());
}

ActionValue As_flash_display_BitmapData::BitmapData_get_rectangle(const BitmapData* self) const
{
	return ActionValue();
}

bool As_flash_display_BitmapData::BitmapData_get_transparent(const BitmapData* self) const
{
	return false;
}

avm_number_t As_flash_display_BitmapData::BitmapData_get_width(const BitmapData* self) const
{
	return avm_number_t(self->getHeight());
}


	}
}
