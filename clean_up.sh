echo Stopping all running containers ...
docker stop $(docker ps -aq)
echo Deleting all docker images ...
docker system prune -a && docker volume rm $(docker volume ls -q)
echo Done