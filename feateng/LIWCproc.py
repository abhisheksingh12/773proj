#!/usr/bin/env python2.7
import re,pickle,sys,traceback

class LIWCproc:
    
    def __init__(self,pathToPickle,readFromFile,pathToLIWC):
        if readFromFile:
            
            file_h = open(pathToLIWC,'r')
            startState = True
            firstPart = False
            secondPart = False
            
            categoryDict = {}
            fullMatchDict = {}
            partialMatchDict = {}
            for line in file_h:
                if startState and '%' in line:
                    firstPart  = True
                    startState = False
                
                elif not startState and '%' in line:
                    firstPart = False
                    secondPart = True
                
                elif firstPart:
                    """
                    Example line:
                    126    posemo
                    """
                    parts = line.split('\t')
                    
                    """
                    rstrip to remove the crap in the
                    strings
                    """
                    parts[0] = parts[0].rstrip()
                    parts[1] = parts[1].rstrip()
                    
                    categoryDict[parts[0]] = parts[1] 
                    
                elif secondPart:
                    """
                    Example line:
                    abandon*    125    127    130    131    137
                    """
                    parts = line.split('\t')
                    
                    parts[0] = parts[0].rstrip()
                    
                    if '*' in parts[0]:
                        tmp_dict = partialMatchDict
                    else:
                        tmp_dict = fullMatchDict
                    
                    tmp_dict[parts[0]] = []
                    
                    for part in parts[1:]:
                        
                        part = part.rstrip()
                        
                        """
                        The spliting is not perfect and it leaves
                        garbage at the end
                        
                        also
                        
                        I am ignoring line 
                        kind    <of>131/125 <of>135/126
                        
                        don't know what this means
                        """
                        
                        if part == '' or re.findall(r'(<of>)|(\()',line) != []:
                            continue
                        
                        
                        tmp_dict[parts[0]].append(categoryDict[part])
                        
            del categoryDict
            file_h.close()
            self.LIWC = {"fullMatch" : fullMatchDict, "partialMatch" : partialMatchDict }    
            pickle.dump(self.LIWC, open(pathToPickle,"wb"))
        else:
            self.LIWC = pickle.load(open(pathToPickle))

    def inquire(self,word):
        """
        I am assumming if there is a full match
        then just return the categories of the 
        full match, otherwise return the categories
        in a partial match
        
        NOTE: Not dealing with multiple matches
        """
        if word in self.LIWC["fullMatch"]:
            return self.LIWC["fullMatch"][word]
        else:
            for key in self.LIWC["partialMatch"]:
                part_word = re.sub(r'\*', "", key)
                if part_word in word:
                    return self.LIWC["partialMatch"][key]
            
            return []
        
        
if __name__ == "__main__":
    
    try:
        
        pickle_f = sys.argv[1]
        #readFromFile = sys.argv[2]
        #readFromFile = {'True': True,
        #          'False': False}[sys.argv[2]]

        if len(sys.argv) == 4:
            readFromFile = True
            resource = sys.argv[2]
            word = sys.argv[3]
        else:
            readFromFile = False
            resource = None
            word = sys.argv[2]
            
            
        ling_res = LIWCproc(pickle_f,readFromFile,resource)
        categoryList = ling_res.inquire(word)
        
        str = "%s\t"%word
        for category in categoryList:
            str += category+"\t"
        str = str[:-2]+"\n"
        sys.stdout.write(str)

    except:
        print ('Usage: pickle_path path_to_LIWC_resource word \nor\n'+\
               'Usage: pickle_path word \n')
        print '-'*60
        traceback.print_exc(file=sys.stdout)
        print '-'*60
        
        
        
