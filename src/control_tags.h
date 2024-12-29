#ifndef __CONTROL_TAGS_H__
#define __CONTROL_TAGS_H__

enum ControlTag
{
    None,
    ACK,
    NACK,
    InfoTracker,
    NoOfFiles,
    FinishedAllDownloads,
    WhoHasThisFile,
    GiveMeHashes,
};

#endif /* __CONTROL_TAGS_H__ */
