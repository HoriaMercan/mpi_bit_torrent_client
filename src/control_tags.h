#ifndef __CONTROL_TAGS_H__
#define __CONTROL_TAGS_H__

enum ControlTag
{
    None,                 /* Do not use. */
    ACK,
    NACK,
    InfoTracker,          /* Used for sending the files that a seed has. */
    NoOfFiles,            /* Number of files. */
    FinishedFileDownload, /* Marks that a client has finished to download certain file. */
    FinishedAllDownloads, /* Marks that a client has finished to download all needed files. */
    WhoHasThisFile,
    GiveMeHashes,
    ReqFile,              /* Request for file segment. */
    AnsFile,              /* Send response for file segment. */
    HowBusyReq,
    HowBusyAns,
};

#endif /* __CONTROL_TAGS_H__ */
