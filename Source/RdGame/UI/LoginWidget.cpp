// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LoginWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Containers/StringConv.h"
#include "GsNetworkSubsystem.h"
#include "Kismet/GameplayStatics.h"

void ULoginWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Connect)
	{
		Btn_Connect->OnClicked.AddDynamic(this, &ULoginWidget::OnConnectClicked);
	}
}

void ULoginWidget::OnConnectClicked()
{
	if (!Input_IP || !Input_Port || !Input_Username)
		return;

	FString IP = Input_IP->GetText().ToString();
	FString PortStr = Input_Port->GetText().ToString();
	FString Username = Input_Username->GetText().ToString();

	if (IP.IsEmpty() || PortStr.IsEmpty() || Username.IsEmpty())
	{
		UpdateStatus(TEXT("모든 정보를 입력해주세요."), FLinearColor::Red);
		return;
	}

	int32 Port = FCString::Atoi(*PortStr);

	UGsNetworkSubsystem* NetSubsystem = GetGameInstance()->GetSubsystem<UGsNetworkSubsystem>();
	if (NetSubsystem)
	{
		UpdateStatus(TEXT("서버 연결 시도 중..."), FLinearColor::Yellow);

		// 1. 서버 접속
		NetSubsystem->Connect(IP, Port);

		// 2. 로그인 응답 핸들러 등록
		// PacketType::S2C_LOGIN_RES = 2
		NetSubsystem->RegisterHandler((uint16)PacketType::S2C_LOGIN_RES, this, &ULoginWidget::HandleLoginRes);

		// 3. 로그인 요청 패킷 전송
		Pkt_LoginReq ReqPkt;
		ReqPkt.size = sizeof(Pkt_LoginReq);
		ReqPkt.type = (uint16)PacketType::C2S_LOGIN_REQ;
		
		// 문자열 복사 (최대 32바이트)
		FTCHARToUTF8 Utf8Username(*Username);
		FMemory::Memzero(ReqPkt.username, sizeof(ReqPkt.username));
		int32 BytesToCopy = FMath::Min((int32)sizeof(ReqPkt.username) - 1, Utf8Username.Length());
		FMemory::Memcpy(ReqPkt.username, Utf8Username.Get(), BytesToCopy);

		// 전송
		TArray<uint8> Buffer;
		Buffer.AddUninitialized(sizeof(Pkt_LoginReq));
		FMemory::Memcpy(Buffer.GetData(), &ReqPkt, sizeof(Pkt_LoginReq));

		NetSubsystem->Send(Buffer);
		
		bIsWaitingForLogin = true;
	}
	else
	{
		UpdateStatus(TEXT("네트워크 서브시스템을 찾을 수 없습니다."), FLinearColor::Red);
	}
}

void ULoginWidget::HandleLoginRes(const TArray<uint8>& PacketData)
{
	if (PacketData.Num() < sizeof(Pkt_LoginRes))
		return;

	const Pkt_LoginRes* ResPkt = reinterpret_cast<const Pkt_LoginRes*>(PacketData.GetData());

	// 메인 스레드에서 UI 업데이트 (GsNetworking의 콜백이 게임 스레드인지 확인 필요, 보통 Tick에서 Dispatch되므로 게임 스레드임)
	if (ResPkt->success)
	{
		FString Msg = FString::Printf(TEXT("로그인 성공! SessionID: %d"), ResPkt->mySessionId);
		UpdateStatus(Msg, FLinearColor::Green);
		
		// TODO: 다음 레벨로 이동하거나 캐릭터 스폰 로직 수행
	}
	else
	{
		UpdateStatus(TEXT("로그인 실패"), FLinearColor::Red);
	}

	// 일회성 핸들러라면 해제 고려 (여기선 유지하거나, 필요 시 해제)
	// GetGameInstance()->GetSubsystem<UGsNetworkSubsystem>()->UnregisterHandler((uint16)PacketType::S2C_LOGIN_RES);
}

void ULoginWidget::UpdateStatus(const FString& Message, FLinearColor Color)
{
	if (Text_Status)
	{
		Text_Status->SetText(FText::FromString(Message));
		Text_Status->SetColorAndOpacity(Color);
	}
}
