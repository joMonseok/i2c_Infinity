import google.generativeai as genai
import os

# 1. API 키 설정
#    "YOUR_API_KEY" 부분을 1단계에서 발급받은 자신의 API 키로 변경하세요.
#    보안을 위해 환경 변수를 사용하는 것이 좋습니다.
#    예: os.environ['GEMINI_API_KEY'] = "YOUR_API_KEY"
try:
    genai.configure(api_key="AIzaSyBHpzx-FBdmZMmS4tRKTsXNqpwK5ZWGFKU")
except AttributeError:
    print("오류: API 키를 설정해주세요. 'YOUR_API_KEY'를 실제 키로 변경해야 합니다.")
    exit()


# 2. 대화 모델 선택 및 초기화
#    'gemini-1.5-pro-latest'는 최신 모델 중 하나입니다.
model = genai.GenerativeModel('gemini-2.5-flash')

firstMessage="""
나
내가 이제부터 json데이터를 줄거야
그 데이터를 기준으로 너가 json 데이터를 수정해서 돌려줄거야
이제 json데이터를 설명할게
id, right, LED가 있을건데
LED는 식물이 잘 자라는 빛을 뿜을거야
id는 건드릴 필요 없고
right는 조도센서값으로 0~1023사이값으로 올거야
LED는 현재 LED밝기로 0~1000까지 조정 가능해
{"id" : 1, "right" : 630, "LED" : 700}
이런 느낌으로 오겠지
그럼 너가 이 데이터를 보고
지금 낮정도 밝기니깐 식물이 자라려면 LED를 꺼야겠다 싶으면
{"id" : 1, "right" : 630, "LED" : 0}
이런식으로 바꾸어서 나한테 전달하는거지
넌 나한테 아무 말 붙이지말고 json만 전달해야해
데이터는 저렇게 하나만 딱 보낼거야
여러개를 한번에 보낼 일 없어
너가 알아서 생각해서 돌려주면 되는거야
이제 위에 내용을 반복하자
혹시 모르니 예시 보내줄게

나
{"id" : 1, "right" : 900, "LED" : 700}
너
{"id" : 1, "right" : 900, "LED" : 0}

```json
```
이런거 달지말고
딱 json만 보내줘

이런식으로만 질문할거고
이런식으로만 대답해
"""

# 3. 채팅 세션 시작 (대화 기록을 유지)
chat = model.start_chat(history=[
    {
        "role": "user",
        "parts": [
            { "text": firstMessage }
        ]
    }
])
print("Gemini 챗봇에 오신 것을 환영합니다! 대화를 시작하세요.")
print("종료하려면 '종료' 또는 'exit'를 입력하세요.")

# 4. 사용자와의 대화 루프
while True:
    try:
        # 사용자 입력 받기
        user_input = input("나: ")

        # 종료 조건 확인
        if user_input.lower() in ["종료", "exit"]:
            print("챗봇을 종료합니다.")
            break

        # 모델에 사용자 메시지 전송 및 응답 받기
        response = chat.send_message(user_input)

        # 모델의 응답 출력
        print(f"Gemini: {response.text}")

    except Exception as e:
        print(f"오류가 발생했습니다: {e}")
        break
        