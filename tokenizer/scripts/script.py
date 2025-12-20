import psycopg2
import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit


DB_CONFIG = {
    "dbname": "crawler",
    "user": "postgres",
    "password": "postgres",
    "host": "localhost",
    "port": 5432
}


SQL_TOKEN_COUNT = """
SELECT COUNT(*) FROM tokens;
"""

SQL_AVG_TOKEN_LENGTH = """
SELECT AVG(LENGTH(token)) FROM tokens;
"""

SQL_TOKEN_FREQUENCIES = """
SELECT SUM(dt.frequency) AS freq
FROM document_tokens dt
GROUP BY dt.token_id
ORDER BY freq DESC;
"""


# Модели
def zipf_law(r, C, s):
    return C / (r ** s)

def mandelbrot_law(r, C, b, s):
    return C / ((r + b) ** s)


def main():
    conn = psycopg2.connect(**DB_CONFIG)
    cur = conn.cursor()

    # Статистика
    cur.execute(SQL_TOKEN_COUNT)
    token_count = cur.fetchone()[0]

    cur.execute(SQL_AVG_TOKEN_LENGTH)
    avg_token_length = cur.fetchone()[0]

    print("СТАТИСТИКА КОРПУСА")
    print("------------------")
    print(f"Количество уникальных токенов: {token_count}")
    print(f"Средняя длина токена: {avg_token_length:.2f}")
    print()

    # Частоты
    cur.execute(SQL_TOKEN_FREQUENCIES)
    freqs = np.array([row[0] for row in cur.fetchall()], dtype=np.float64)

    cur.close()
    conn.close()

    ranks = np.arange(1, len(freqs) + 1)


    # закон Ципфа

    popt_zipf, _ = curve_fit(
        zipf_law,
        ranks,
        freqs,
        p0=[freqs[0], 1.0],
        maxfev=10000
    )

    C_zipf, s_zipf = popt_zipf

    print("ЗАКОН ЦИПФА")
    print("-----------")
    print(f"C = {C_zipf:.4e}")
    print(f"s = {s_zipf:.4f}")
    print()

    # Закон Мандельброта
    popt_mandelbrot, _ = curve_fit(
        mandelbrot_law,
        ranks,
        freqs,
        p0=[freqs[0], 1.0, 1.0],
        maxfev=20000
    )

    C_m, b_m, s_m = popt_mandelbrot

    print("ЗАКОН МАНДЕЛЬБРОТА")
    print("------------------")
    print(f"C = {C_m:.4e}")
    print(f"b = {b_m:.4f}")
    print(f"s = {s_m:.4f}")
    print()

    # ГРАФИКИ
    plt.figure(figsize=(9, 6))

    plt.loglog(ranks, freqs, marker=".", linestyle="none", label="Корпус")
    plt.loglog(ranks, zipf_law(ranks, C_zipf, s_zipf),
               label="Закон Ципфа")
    plt.loglog(ranks, mandelbrot_law(ranks, C_m, b_m, s_m),
               label="Закон Мандельброта")

    plt.xlabel("Ранг термина")
    plt.ylabel("Частота")
    plt.title("Распределение терминов (log-log)")
    plt.legend()
    plt.grid(True, which="both", ls="--", alpha=0.4)

    plt.tight_layout()
    plt.savefig("zipf_mandelbrot.png", dpi=300)
    plt.close()



if __name__ == "__main__":
    main()
