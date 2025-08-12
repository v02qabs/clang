curl -v -X POST https://api.line.me/v2/bot/message/push \
-H 'Content-Type: application/json' \
-H 'Authorization: Bearer {"fXJOw1InhgemyDfv7efRsu0byQQptBn962c8CgtVzYbpLktUfxE/7TxnRLwyY6bTIW483OxxE5HQBo/oMAUOsujLplxB16j5F9sV+jZe4mvOOzaCnpL5dzS6YJ756eUjjEMgfDHsRy+1xsmbeqUGoQdB04t89/1O/w1cDnyilFU="}' \
-d '{
    "to": "takosuguytfx",
    "messages":[
        {
            "type":"text",
            "text":"Hello, world1"
        },
        {
            "type":"text",
            "text":"Hello, world2"
        }
    ]
}'
