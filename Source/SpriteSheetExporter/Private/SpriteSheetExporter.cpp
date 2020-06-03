// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpriteSheetExporter.h"
#include "IAssetRegistry.h"
#include "Engine/Texture.h"
#include "AssetRegistryModule.h"
#include "ModuleManager.h"
#include "Engine/Texture2D.h"
#include "RenderResource.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "MessageLog.h"
#include "BufferArchive.h"
#include "FileManager.h"
#include "../Plugins/2D/Paper2D/Source/PaperSpriteSheetImporter/Private/PaperSpriteSheet.h"

#define LOCTEXT_NAMESPACE "FSpriteSheetExporterModule"

// Save as a PNG file to path, with given raw data and image size
void ExportPNG(const FString& TotalFileName, const TArray64<uint8>& RawData, const int32 width, const int32 height)
{
	FText PathError;
	FPaths::ValidatePath(TotalFileName, &PathError);
	if (!PathError.IsEmpty())
	{
		FMessageLog("Blueprint").Warning(FText::Format(LOCTEXT("InvalidFilePath", "Invalid file path provided: '{0}'"), PathError));
		return;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::Get().LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> PNGImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	PNGImageWrapper->SetRaw(RawData.GetData(), RawData.GetAllocatedSize(), width, height, ERGBFormat::BGRA, 8);

	const TArray64<uint8>& PNGData = PNGImageWrapper->GetCompressed(100);

	FBufferArchive Buffer;
	Buffer.Serialize((void*)PNGData.GetData(), PNGData.GetAllocatedSize());

	FArchive* fileWriter = IFileManager::Get().CreateFileWriter(*TotalFileName);
	fileWriter->Serialize(const_cast<uint8*>(Buffer.GetData()), Buffer.Num());
	delete fileWriter;
}


// Fetch raw data of a sprite in a texture with given info
void TrimSprite(const TArray64<uint8>& RawData, const int32 Width, const int32 Height, const FVector2D& UV, const FVector2D& Size, const int32 Span, const bool bRotated, TArray64<uint8>& out)
{
	const int32 w = Size.X, h = Size.Y;
	const int32 x = UV.X, y = UV.Y;
	const uint8* head = RawData.GetData();

	out.Reserve(w * h * Span);

	if (bRotated)
	{
		for (int i = 0; i < w; ++i)
			for (int j = 0; j < h; ++j) 
			{
				out.Append(head + Span * ((y + j) * Width + x + w - i - 1), Span);
			}
	}
	else
	{
		for (int i = 0; i < h; ++i)
			out.Append(head + ((y + i) * Width + x) * Span, w * Span);
	}
}


// Export a single atlas
bool ExportAtlas(const UPaperSpriteSheet* sprite_sheet, const FString& exportPath)
{
	UTexture2D* tex = sprite_sheet->Texture;

	TArray64<uint8> RawData;


	// export texture
	if (!tex->Source.GetMipData(RawData, 0))
		return false;
	const FString TotalFileName = FPaths::Combine(exportPath, tex->GetFName().ToString()) + TEXT("/");
	ExportPNG(TotalFileName + sprite_sheet->TextureName, RawData, tex->Source.GetSizeX(), tex->Source.GetSizeY());


	// export sprites
	for (auto& softRef : sprite_sheet->Sprites)
	{
		const UPaperSprite* sprite = softRef.LoadSynchronous();

		TArray64<uint8> spriteData;
		TrimSprite(RawData, tex->Source.GetSizeX(), tex->Source.GetSizeY(), 
			sprite->GetSourceUV(), sprite->GetSourceSize(), FTextureSource::GetBytesPerPixel(tex->Source.GetFormat()), 
			sprite->IsRotatedInSourceImage(), spriteData);

		if(sprite->IsRotatedInSourceImage())
			ExportPNG(TotalFileName + sprite->GetFName().ToString() + TEXT(".png"), spriteData, sprite->GetSourceSize().Y, sprite->GetSourceSize().X);
		else
			ExportPNG(TotalFileName + sprite->GetFName().ToString() + TEXT(".png"), spriteData, sprite->GetSourceSize().X, sprite->GetSourceSize().Y);
	}


	return true;
}


// Export all Atlases in project
bool ExportAllAtlas(const FString& exportPath)
{
	bool bSuccess = true;


	// Fetch all Sprite Sheet assets in project's content directory
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	auto CachedAssetRegistry = &AssetRegistryModule.Get();

	FARFilter filter;
	filter.ClassNames.Add(TEXT("PaperSpriteSheet"));
	filter.PackagePaths.Add(TEXT("/Game"));
	filter.bRecursivePaths = true;

	TArray<FAssetData> sprite_sheets;
	CachedAssetRegistry->GetAssets(filter, sprite_sheets);


	for (auto& asset : sprite_sheets) 
	{
		bSuccess &= ExportAtlas((UPaperSpriteSheet*)(asset.GetAsset()), exportPath);	//todo: Cast<>
	}

	return bSuccess;
}


bool FSpriteSheetExporterModule::Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	// Register ExportAllAtlas to console command
	FString token = FParse::Token(Cmd, false);
	if (token == TEXT("ExportAllAtlas"))
		ExportAllAtlas(FPaths::ProjectSavedDir() + TEXT("Atlases"));	//todo: parse from command line
	else
		return false;

	return true;
}


void FSpriteSheetExporterModule::StartupModule()
{
}

void FSpriteSheetExporterModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSpriteSheetExporterModule, SpriteSheetExporter)