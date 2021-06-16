import boto3
from boto3.dynamodb.types import Binary

dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('cspot')

def put_item(key, data):
    print(key, data)
    table.put_item(
        Item = {
            'Id': key,
            'Data': Binary(data),
        }
    )
    return 0;
