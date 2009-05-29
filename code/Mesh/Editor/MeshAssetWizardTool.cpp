#include "Mesh/Editor/MeshAssetWizardTool.h"
#include "Mesh/Editor/MeshAsset.h"
#include "Editor/Editor.h"
#include "Database/Database.h"
#include "Database/Group.h"
#include "Database/Instance.h"
#include "Ui/FileDialog.h"
#include "I18N/Text.h"
#include "Core/Misc/StringUtils.h"
#include "Core/Log/Log.h"

namespace traktor
{
	namespace mesh
	{

T_IMPLEMENT_RTTI_SERIALIZABLE_CLASS(L"traktor.mesh.MeshAssetWizardTool", MeshAssetWizardTool, editor::WizardTool)

std::wstring MeshAssetWizardTool::getDescription() const
{
	return i18n::Text(L"MESHASSET_WIZARDTOOL_DESCRIPTION");
}

bool MeshAssetWizardTool::launch(ui::Widget* parent, editor::Editor* editor, db::Group* group)
{
	// Select source model.
	ui::FileDialog fileDialog;
	if (!fileDialog.create(parent, i18n::Text(L"MESHASSET_WIZARDTOOL_FILE_TITLE"), L"All files;*.*"))
		return 0;

	Path fileName;
	if (fileDialog.showModal(fileName) != ui::DrOk)
	{
		fileDialog.destroy();
		return true;
	}
	fileDialog.destroy();

	// Create source asset.
	Ref< MeshAsset > asset = gc_new< MeshAsset >();
	asset->setFileName(fileName);
	asset->m_meshType = MeshAsset::MtStatic;

	// Create asset instance.
	Ref< db::Instance > assetInstance = group->createInstance(
		fileName.getFileNameNoExtension(),
		asset,
		db::CifReplaceExisting | db::CifKeepExistingGuid
	);
	if (!assetInstance)
		return false;

	if (!assetInstance->commit())
		return false;

	return true;
}

	}
}
