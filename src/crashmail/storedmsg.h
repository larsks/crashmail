struct StoredMsg
{
   uchar From[36];
   uchar To[36];
   uchar Subject[72];
   uchar DateTime[20];
   UINT16 TimesRead;
   UINT16 DestNode;
   UINT16 OrigNode;
   UINT16 Cost;
   UINT16 OrigNet;
   UINT16 DestNet;
   UINT16 DestZone;
   UINT16 OrigZone;
   UINT16 DestPoint;
   UINT16 OrigPoint;
   UINT16 ReplyTo;
   UINT16 Attr;
   UINT16 NextReply;
   /* Text */
};
