## Gcloud Deployment Steps

1. Buka powershell di direktori ini
2. 'gcloud init'
3. 'gcloud config set project <nama_project>
4. Upload image : 'gcloud builds submit --tag gcr.io/<project_id>/<nama_images_bebas>' 
5. Deploy ke Cloud Run : 'gcloud run deploy --image gcr.io/<project_id>/<nama_images_tadi> --platform managed'
6. Service name = 'energyforecastlstm' / bebas kalau baru buat
7. region = nomor 10. asia-southeast2




