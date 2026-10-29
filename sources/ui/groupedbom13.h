/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef QET_GROUPEDBOM13_H
#define QET_GROUPEDBOM13_H
#include <QString>
namespace QetFormatting {
inline QString groupedBomQuery(bool sumQuantity,bool references,bool numbering,bool auxiliaries,bool meaningfulOnly=true) {
    auto article=[meaningfulOnly](const QString &suffix){
        const QString s=suffix;
        QString query=QString("SELECT COALESCE(manufacturer%1,'') AS manufacturer, COALESCE(manufacturer_reference%1,'') AS article, COALESCE(NULLIF(TRIM(description%1),''),designation%1,'') AS name, label, COALESCE(unity%1,'') AS unit, CAST(REPLACE(COALESCE(NULLIF(TRIM(quantity%1),''),'1'),',','.') AS REAL) AS qty, COALESCE(NULLIF(element_uuid,''),'row:'||CAST(ROW_NUMBER() OVER () AS TEXT))||':%1' AS itemkey FROM element_nomenclature_view").arg(s);
        if(meaningfulOnly||!s.isEmpty())query+=QString(" WHERE TRIM(COALESCE(manufacturer_reference%1,''))<>'' OR TRIM(COALESCE(designation%1,''))<>'' OR TRIM(COALESCE(description%1,''))<>''").arg(s);
        return query;
    };
    QString source=article("");if(auxiliaries)for(int i=1;i<=4;++i)source+=" UNION ALL "+article("_auxiliary"+QString::number(i));
    QString grouped="SELECT manufacturer, article, MIN(name) AS name, unit, "+QString(sumQuantity?"SUM(qty)":"COUNT(*)")+" AS qty, GROUP_CONCAT(DISTINCT label) AS refs FROM ("+source+") GROUP BY manufacturer,article,unit,CASE WHEN TRIM(article)='' THEN COALESCE(label,'')||itemkey ELSE '' END";
    QString fields=numbering?"ROW_NUMBER() OVER (ORDER BY manufacturer,article,name,refs) AS \"No.\", ":"";
    fields+="qty AS \"Qty\",name AS \"Name\",article AS \"Part number\"";if(references)fields+=",refs AS \"References\"";fields+=",manufacturer AS \"Manufacturer\",unit AS \"Unit\"";
    return QString("SELECT /*qet-bom13 qty=%1 refs=%2 seq=%3 aux=%4 data=%5*/ ").arg(sumQuantity?1:0).arg(references?1:0).arg(numbering?1:0).arg(auxiliaries?1:0).arg(meaningfulOnly?1:0)+fields+" FROM ("+grouped+") ORDER BY manufacturer,article,name,refs";
}
}
#endif
