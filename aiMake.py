# train_led_multi_input.py
import numpy as np
import pandas as pd
from sklearn.linear_model import LinearRegression

CSV_PATH = "sensor_led_multi.csv"  # right, soil, temp, led 있는 CSV

def load_data_multi(path: str):
    df = pd.read_csv(path)

    required_cols = {"right", "soil", "temp", "led"}
    if not required_cols.issubset(df.columns):
        raise ValueError(f"CSV 파일에 {required_cols} 컬럼이 있어야 합니다: {required_cols}")

    # 결측값(NaN) 있으면 기본값 넣기 (예시)
    df["soil"] = df["soil"].fillna(50.0)   # 토양수분 없으면 50% 로 가정
    df["temp"] = df["temp"].fillna(25.0)   # 온도 없으면 25도로 가정

    X = df[["right", "soil", "temp"]].to_numpy(dtype=float)  # (N, 3)
    y = df["led"].to_numpy(dtype=float)                       # (N,)
    return X, y

def main():
    X, y = load_data_multi(CSV_PATH)
    print(f"[로드] 샘플 수: {len(X)}")

    model = LinearRegression()
    model.fit(X, y)

    # 계수: y ≈ b0 + b1*right + b2*soil + b3*temp
    b0 = model.intercept_
    b1, b2, b3 = model.coef_

    print("\n=== 학습된 선형 모델 ===")
    print(f"LED ≈ {b0:.6f} + {b1:.6f}*right + {b2:.6f}*soil + {b3:.6f}*temp")

    print("\n=== C 코드 예시 ===")
    print("float compute_led(float right, float soil, float temp) {")
    print(f"    const float b0 = {b0:.6f}f;")
    print(f"    const float b1 = {b1:.6f}f;")
    print(f"    const float b2 = {b2:.6f}f;")
    print(f"    const float b3 = {b3:.6f}f;")
    print("    float led = b0 + b1*right + b2*soil + b3*temp;")
    print("    if (led < 0.0f) led = 0.0f;")
    print("    if (led > 1023.0f) led = 1023.0f;")
    print("    return led;")
    print("}")

    # 예측 테스트 (앞 5개)
    print("\n=== 예측 테스트 (앞 5개) ===")
    y_pred = model.predict(X)
    for i in range(min(5, len(X))):
        print(f"입력={X[i]}, 실제 LED={y[i]:.2f}, 예측 LED={y_pred[i]:.2f}, 오차={y_pred[i] - y[i]:.2f}")

if __name__ == "__main__":
    main()
