# Каждый документ имеет уникальный id или url
ground_truth = {
    "технологичный костюм": ["https://anime-characters-fight.fandom.com/ru/wiki/Бэтмен_(Arkham_Series)"],
    "жидкая броня": ["https://anime-characters-fight.fandom.com/ru/wiki/Бэтмен_(Arkham_Series)"],
    "Бэтмобиль синхронизация": ["https://anime-characters-fight.fandom.com/ru/wiki/Бэтмен_(Arkham_Series)"],
    "костюм экстремально низкие температуры": ["https://anime-characters-fight.fandom.com/ru/wiki/Бэтмен_(Arkham_Series)"],
    "бэтаранги": ["https://anime-characters-fight.fandom.com/ru/wiki/Бэтмен_(Arkham_Series)"]
}

# Соответствие документов с текстовыми блоками
documents = {
    "https://anime-characters-fight.fandom.com/ru/wiki/Бэтмен_(Arkham_Series)": """Костюм v8.04 - это новый, основной, улучшенный и технологичный костюм Бэтмена...
               ...катапультирование происходит моментально.""",
    "https://anime-characters-fight.fandom.com/ru/wiki/Бэтмен_(Arkham_Series)": """Костюм XE - созданный Брюсом костюм для нахождения в экстремально низких температурах...
               ...повышая их убойность."""
}


import requests

server_url = "http://localhost:8081/search?q="

K = 5  # топ-K результатов для оценки

def precision_at_k(retrieved, relevant, k):
    retrieved_k = retrieved[:k]
    relevant_set = set(relevant)
    if not retrieved_k:
        return 0.0
    return len([d for d in retrieved_k if d in relevant_set]) / len(retrieved_k)

def recall_at_k(retrieved, relevant, k):
    retrieved_k = retrieved[:k]
    relevant_set = set(relevant)
    if not relevant_set:
        return 0.0
    return len([d for d in retrieved_k if d in relevant_set]) / len(relevant_set)

def f1(prec, rec):
    if prec + rec == 0:
        return 0.0
    return 2 * (prec * rec) / (prec + rec)

# Запуск тестов
precisions = []
recalls = []
f1_scores = []

for query, relevant_docs in ground_truth.items():
    response = requests.get(server_url + query)
    if response.status_code != 200:
        print(f"Ошибка запроса: {query}")
        continue
    
    results = response.json()
    retrieved_docs = [r["url"] for r in results] 

    prec = precision_at_k(retrieved_docs, relevant_docs, K)
    rec = recall_at_k(retrieved_docs, relevant_docs, K)
    f1_score = f1(prec, rec)

    print(f"Запрос: '{query}'")
    print(f"  Precision@{K}: {prec:.2f}, Recall@{K}: {rec:.2f}, F1@{K}: {f1_score:.2f}\n")

    precisions.append(prec)
    recalls.append(rec)
    f1_scores.append(f1_score)

# Средние метрики по всем запросам
if precisions:
    print("Средние показатели:")
    print(f"  Precision@{K}: {sum(precisions)/len(precisions):.2f}")
    print(f"  Recall@{K}: {sum(recalls)/len(recalls):.2f}")
    print(f"  F1@{K}: {sum(f1_scores)/len(f1_scores):.2f}")
