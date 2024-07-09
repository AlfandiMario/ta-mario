from datetime import datetime, timedelta
from fastapi import FastAPI
import pandas as pd
import numpy as np
from sklearn.preprocessing import MinMaxScaler
from tensorflow.keras.models import Sequential, load_model
from tensorflow.keras.layers import LSTM, Dense, Dropout
import requests
import os
import uvicorn
import logging

isDebug = False
if isDebug:
    post_url = 'http://127.0.0.1:8000/api/receive-forecast'
else:
    post_url = 'https://iotlab-uns.com/smart-bms/public/api/receive-forecast'

n_steps = 7
headers = { "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"}

app = FastAPI()

@app.get("/")
def read_root():
    return {"Hello": "World"}

@app.get("/predict")
async def predict_energy():
    try:
        # Akuisisi Data
        url = 'https://iotlab-uns.com/smart-bms/public/api/daily-energy-reversed'
        response = requests.get(url, headers=headers)
        data = response.json()

        # Convert date strings ke objek datetime agar bisa diidentifikasi
        for record in data:
            record["date"] = datetime.strptime(record["date"], "%Y-%m-%d").date()

        today = datetime.now().date()
        lastRabu = today - timedelta(days=today.weekday() - 2) # Rabu minggu terakhir sebelum minggu ini
        print("Last Thursday:", lastRabu)

        filtered_data = []
        for record in data:
            if record["date"] <= lastRabu:
                filtered_data.append(record)

        # Convert objek datetime ke date string lagi agar bisa jadi JSON rapi
        for record in filtered_data:
            record["date"] = record["date"].strftime("%Y-%m-%d")

        # Extract 'date' dan 'today_energy' dari filtered_data JSON
        dates = [datetime.strptime(record['date'], "%Y-%m-%d") for record in filtered_data]
        energies = [record['today_energy'] for record in filtered_data]
        energies = np.array(energies).reshape(-1, 1)

        # Preprocess the data
        scaler = MinMaxScaler()
        energies_normalized = scaler.fit_transform(energies)

        # Load .h5 model
        model = load_model("generatedModel.h5")

        # Membuat prediksi
        predictions = []
        current_batch = energies_normalized[-n_steps:].reshape((1, n_steps, 1))

        # Prediksi selama 14 hari dimulai pada hari Kamis minggu kemarin
        for i in range(14):
            current_pred = model.predict(current_batch)[0]
            predictions.append(current_pred)
            current_batch = np.append(current_batch[:, 1:, :], [[current_pred]], axis=1)

        # Inverse transform predictions
        predictions_actual = scaler.inverse_transform(predictions)
        for i in range (len(predictions_actual)):
            if predictions_actual[i] < 0:
                predictions_actual[i] =+ 300.00

        # Convert predictions menjadi list untuk serialisasi JSON
        predictions_list = predictions_actual.flatten().tolist()

        # Hasilnya JSON format dengan tanggal mulai Kamis minggu ini
        results = []
        last_date = today - timedelta(days=today.weekday()-3) # Mulai dari Kamis
        for i in range(len(predictions)):
            date = last_date + timedelta(days=i)
            prediction = predictions_list[i]
            results.append({"date": date.strftime("%Y-%m-%d"), "prediction": prediction})

        # Mengirim hasil prediksi ke API Web dengan bentuk JSON melalui HTTP POST Request
        response = requests.post(post_url, json=results, headers=headers)

        # Apakah POST request sukses
        if response.status_code == 200:
            return results
        else:
            return {"message": "Failed to send predictions to {post_url}"}

    except Exception as e:
        logging.error("Error during prediction:", exc_info=True)
        raise  # Re-raise the exception for the error handler


@app.get("/modelling")
async def update_model():
    # Fetch JSON data from the API
    headers = { "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"}
    url = 'https://iotlab-uns.com/smart-bms/public/api/daily-energy-reversed'
    response = requests.get(url, headers=headers)
    data = response.json()

    # Convert date strings ke objek datetime agar bisa diidentifikasi
    for record in data:
        record["date"] = datetime.strptime(record["date"], "%Y-%m-%d").date()

    today = datetime.now().date()
    lastRabu = today - timedelta(days=today.weekday() - 2) # Rabu minggu terakhir sebelum minggu ini

    filtered_data = []
    for record in data:
        if record["date"] <= lastRabu:
            filtered_data.append(record)

    # Convert objek datetime ke date string lagi agar bisa jadi JSON rapi
    for record in filtered_data:
        record["date"] = record["date"].strftime("%Y-%m-%d")

    # Extract 'date' dan 'today_energy' dari filtered_data JSON
    dates = [datetime.strptime(record['date'], "%Y-%m-%d") for record in filtered_data]
    energies = [record['today_energy'] for record in filtered_data]

    train_data = np.array(energies).reshape(-1,1)
    last_train_date = dates[-1]

    # Normalize the data
    scaler = MinMaxScaler()
    train_data_normalized = scaler.fit_transform(train_data)
    
    # Prepare the data for LSTM
    def prepare_data(data, n_steps):
        X, y = [], []
        for i in range(len(data)):
            end_ix = i + n_steps
            if end_ix > len(data)-1:
                break
            seq_x, seq_y = data[i:end_ix], data[end_ix]
            X.append(seq_x)
            y.append(seq_y)
        return np.array(X), np.array(y)
    
    # Prepare the data for LSTM
    X_train, y_train = prepare_data(train_data_normalized, n_steps)
    
    # Reshape the data for LSTM [samples, timesteps, features]
    X_train = X_train.reshape((X_train.shape[0], X_train.shape[1], 1))

    # Define the LSTM model architecture
    model = Sequential()
    model.add(LSTM(units=128, activation='relu', return_sequences=True, input_shape=(n_steps, 1)))
    model.add(Dropout(0.1))
    model.add(LSTM(units=64, activation='relu', return_sequences=True))
    model.add(Dropout(0.1))
    model.add(LSTM(units=16, activation='relu'))
    model.add(Dense(1))
    model.compile(optimizer='adam', loss='mse')
    
    # Train and save the model
    model.fit(X_train, y_train, epochs=200, verbose=0)
    model.save('generatedModel.h5')

    # Evaluate the model
    train_loss = model.evaluate(X_train, y_train, verbose=0)
    
    return {"message": "Model updated successfully","last_train_date : ": last_train_date ,"train_loss": train_loss}

if __name__ == "__main__":
    uvicorn.run(app, port=int(os.environ.get("PORT", 8080)), host="0.0.0.0")