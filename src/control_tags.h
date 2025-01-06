#ifndef __CONTROL_TAGS_H__
#define __CONTROL_TAGS_H__

enum ControlTag
{
    None,                 /* Do not use. */
    ACK,
    NACK,
    InfoTracker,          /* Used for sending the files that a seed has. */
    NoOfFiles,            /* Send number of files a client initially owns to the tracker. */
    FinishedFileDownload, /* Marks that a client has finished to download certain file. */
    FinishedAllDownloads, /* Marks that a client has finished to download all needed files. */
    WhoHasThisFile,       /* Ask the tracker who owns/have partial elements of a file. */
    GiveMeHashes,         /* Request hashes of a file from the tracker. */
    ReqFile,              /* Request for file segment. */
    AnsFile,              /* Send response for file segment. */
    HowBusyReq,           /* Send request to ask another client how busy it is. */
    HowBusyAns,           /* Send the busy score for the aforementioned request. */
};

#endif /* __CONTROL_TAGS_H__ */
